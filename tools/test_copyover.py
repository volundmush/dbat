#!/usr/bin/env python3
"""test_copyover.py BINARY [SCENARIO...]
Runs real servers with real clients through copyovers, on scratch copies of the test fixture.
Each scenario prints its checks; the exit status says whether they all passed."""
import os, re, shutil, signal, socket, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIXTURE = os.path.join(ROOT, "libs/dbat/tests/fixtures")
BINARY = os.path.abspath(sys.argv[1])
ACCOUNTS = [("Zorbexb", "Pa55word"), ("Zorbexc", "Pa55word")]  # both Implementors
WORK = tempfile.mkdtemp(prefix="dbat-copyover-", dir=os.environ.get("TMPDIR"))
results = {}
servers = []
next_port = 8900 + os.getpid() % 500


def check(name, ok, detail=""):
    print(("  ok   " if ok else "  FAIL ") + name + (f": {detail}" if detail != "" and not ok else ""), flush=True)
    results[name] = bool(ok)


class Client:
    """A player's connection."""

    def __init__(self, port):
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=5)
        self.sock.setblocking(False)
        self.received = b""
        self.closed = False

    def pump(self, seconds=0.3):
        end = time.time() + seconds
        while time.time() < end and not self.closed:
            try:
                data = self.sock.recv(65536)
                if data == b"":
                    self.closed = True
                self.received += data
            except BlockingIOError:
                time.sleep(0.02)
            except OSError:
                self.closed = True

    def send(self, line, wait=0.35):
        try:
            self.sock.sendall(line.encode() + b"\r\n")
        except OSError:
            self.closed = True
        self.pump(wait)

    def text(self):
        return self.received.decode("latin-1")

    def take(self):
        text = self.text()
        self.received = b""
        return text

    def wait_for(self, what, seconds=45):
        end = time.time() + seconds
        while time.time() < end and not self.closed and what not in self.text():
            self.pump(0.2)
        return what in self.text()

    def ask(self, line, wait=0.5):
        self.take()
        self.send(line, wait)
        return self.text()


class Server:
    """A real server on its own copy of the fixture, with two accounts."""

    def __init__(self, name, data="data", args=None, cwd=""):
        global next_port
        next_port += 1
        self.port = next_port
        self.home = os.path.join(WORK, name)
        self.data = os.path.join(self.home, data)
        self.cwd = os.path.join(self.home, cwd)
        os.makedirs(self.data)
        shutil.copytree(os.path.join(FIXTURE, "world"), self.data, dirs_exist_ok=True)
        shutil.copytree(os.path.join(FIXTURE, "accounts"), self.data, dirs_exist_ok=True)
        with open(os.path.join(self.data, "surface.map"), "w") as f:
            for _ in range(201):
                f.write("300 " * 201 + "\n")
        with open(os.path.join(self.data, "etc/config"), "a") as f:
            f.write("load_into_inventory = 1\n")
        self.log_path = os.path.join(self.home, "output.log")
        self.args = args if args is not None else ["-d", os.path.relpath(self.data, self.cwd)]
        self.binary = BINARY
        self.boots_seen = 0
        servers.append(self)

    def start(self, before_exec=None):
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="print_stacktrace=1")
        self.output = open(self.log_path, "ab")
        self.process = subprocess.Popen([self.binary, *self.args, str(self.port)], cwd=self.cwd, stdout=self.output,
                                        stderr=subprocess.STDOUT, env=env, preexec_fn=before_exec)
        return self.wait_up()

    def log(self):
        text = ""
        for path in (self.log_path, *getattr(self, "other_logs", ())):
            if os.path.exists(path):
                with open(path, errors="replace") as f:
                    text += f.read()
        return text

    def wait_up(self, seconds=60):
        end = time.time() + seconds
        while time.time() < end and self.process.poll() is None:
            boots = self.log().count("Entering game loop")
            if boots > self.boots_seen:
                self.boots_seen = boots
                return True
            time.sleep(0.1)
        return False

    def login(self, account):
        client = Client(self.port)
        client.pump(0.4)
        for line in (*account, "1", "", "1"):
            client.send(line, 0.4)
        return client

    def copyover(self, client):
        client.send("copyover", 0.1)
        return self.wait_up()

    def hold_next_boot(self):
        """Makes the next process to boot wait partway through, by swapping the surface map for a pipe."""
        self.map_path = os.path.join(self.data, "surface.map")
        os.replace(self.map_path, self.map_path + ".real")
        os.mkfifo(self.map_path)

    def release_boot(self, seconds=30):
        """Returns False if no process ever came to wait."""
        end = time.time() + seconds
        while True:
            try:
                pipe = os.open(self.map_path, os.O_WRONLY | os.O_NONBLOCK)
                break
            except OSError:
                if time.time() > end:
                    return False
                time.sleep(0.01)
        os.set_blocking(pipe, True)
        # Later opens get the ordinary file; the one that's waiting still has the pipe.
        with open(self.map_path + ".real", "rb") as f:
            contents = f.read()
        os.replace(self.map_path + ".real", self.map_path)
        try:
            os.write(pipe, contents)
        except BrokenPipeError:
            pass
        os.close(pipe)
        return True

    def descriptors(self):
        return len(os.listdir(f"/proc/{self.process.pid}/fd"))

    def finish(self):
        """Stops the server, and checks that it never reported a memory error."""
        if self.process.poll() is None:
            self.process.send_signal(signal.SIGTERM)
            try:
                self.process.wait(20)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        self.output.close()
        errors = [line for line in self.log().splitlines() if "ERROR: AddressSanitizer" in line or "runtime error" in line]
        check("no memory errors", not errors, errors[:2])
        shutil.rmtree(self.home, ignore_errors=True)


def scenario_players_come_back():
    server = Server("basic")
    check("boots", server.start())
    a, b = server.login(ACCOUNTS[0]), server.login(ACCOUNTS[1])
    a.send("goto 301")
    a.send("load obj 301")
    check("has the sword first", "plain sword" in a.ask("inventory"))
    b.send("goto 304")

    client_during_boot = None
    a.send("copyover", 0.1)
    client_during_boot = Client(server.port)  # the listening socket carries on through the exec
    check("comes back", server.wait_up())
    check("told it worked", a.wait_for("Folding complete") and b.wait_for("Folding complete"))

    check("keeps the room", "Training Yard" in a.ask("look"))
    check("keeps the inventory", "plain sword" in a.ask("inventory"))
    check("keeps the other player's room", "Back Lane" in b.ask("look"))
    client_during_boot.pump(1)
    check("someone connecting during the boot gets in", len(client_during_boot.text()) > 10)

    # the menus use account fields that a login normally sets up
    b.send("quit", 0.8)
    for line in ("", "1", "", "2", "0", "1", "", "1"):
        b.send(line, 0.3)
    check("menus work afterwards", server.process.poll() is None)
    check("can copyover again", server.copyover(a))
    check("still connected", a.wait_for("Folding complete") and not a.closed)
    server.finish()


def scenario_switched_immortal():
    server = Server("switched")
    check("boots", server.start())
    a, b = server.login(ACCOUNTS[0]), server.login(ACCOUNTS[1])
    a.send("goto 301")
    a.send("switch dummy", 0.5)
    check("comes back", server.copyover(b))
    time.sleep(0.5)
    check("no damaged file", "malformed" not in server.log())
    check("switched immortal comes back as themselves", a.wait_for("Folding complete") and "Kelvarin" in a.ask("score", 0.8))
    check("other player comes back", b.wait_for("Folding complete"))
    server.finish()


def scenario_builder_in_an_editor():
    server = Server("editor")
    check("boots", server.start())
    a, b = server.login(ACCOUNTS[0]), server.login(ACCOUNTS[1])
    a.send("redit 300", 0.5)
    check("comes back", server.copyover(b))
    check("told the edit is lost", "editing has been lost" in a.text() or a.wait_for("editing has been lost", 2))
    check("keeps their connection", a.wait_for("Folding complete") and not a.closed)
    check("can play on", "Town Square" in a.ask("look") or len(a.text()) > 20)
    server.finish()


def scenario_link_dead_player():
    server = Server("linkdead")
    check("boots", server.start())
    a, b = server.login(ACCOUNTS[0]), server.login(ACCOUNTS[1])
    b.send("load obj 302", 0.4)
    b.sock.close()
    time.sleep(0.4)
    a.send("", 0.2)
    path = os.path.join(server.data, "plrobjs/K-O/kelvarix.new")
    check("comes back", server.copyover(a))
    with open(path) as f:
        check("link-dead player's new object was saved", "#302" in f.read())
    server.finish()


def scenario_how_it_was_started():
    server = Server("nested", data="deep/er/data", cwd="")
    server.args = ["-d", "deep/er/data", "-q", "-s", "-o", "my.log"]
    server.other_logs = (os.path.join(server.home, "my.log"),)
    check("boots from a nested directory, with options", server.start())
    a = server.login(ACCOUNTS[0])
    check("comes back", server.copyover(a))
    log = server.log()
    with open(f"/proc/{server.process.pid}/cmdline") as f:
        arguments = f.read().split("\0")
    check("options are kept", all(option in arguments for option in ("-q", "-s", "my.log", "deep/er/data")), arguments)
    check("-o log file keeps what came before it", log.count("Boot db -- BEGIN") == 2, log.count("Boot db -- BEGIN"))
    check("player comes back", a.wait_for("Folding complete"))
    server.finish()


def scenario_wizlock():
    server = Server("wizlock")
    check("boots", server.start())
    a = server.login(ACCOUNTS[0])
    a.send("wizlock 3", 0.4)
    check("comes back", server.copyover(a))
    a.wait_for("Folding complete")
    a.pump(0.5)
    text = a.ask("wizlock")
    check("the game stays locked", "completely open" not in text, text[:80])
    server.finish()


def scenario_no_descriptor_leak():
    server = Server("leak")
    check("boots", server.start())
    a = server.login(ACCOUNTS[0])
    counts = []
    for _ in range(4):
        check("comes back", server.copyover(a))
        a.pump(0.5)
        counts.append(server.descriptors())
    check("descriptors stay level", counts[-1] == counts[0], counts)
    server.finish()


def scenario_binary_is_gone():
    server = Server("binary")
    server.binary = os.path.join(server.home, "dbat-copy")
    shutil.copy(BINARY, server.binary)
    check("boots", server.start())
    a = server.login(ACCOUNTS[0])
    a.send("load obj 301")
    os.remove(server.binary)
    a.send("copyover", 2)
    check("the game carries on", server.process.poll() is None, server.process.poll())
    check("the immortal is told why", "Copyover aborted" in a.text() or "Copyover aborted" in server.log())
    check("the player's objects are still there", "plain sword" in a.ask("inventory"))
    check("nothing is left behind", not os.path.exists(os.path.join(server.data, "copyover.dat")))
    server.finish()


def scenario_damaged_file():
    def truncated(text):
        return text.split("\n")[0] + "\n"

    def garbage_first(text):
        return "this is not a line\n" + text

    def wrong_descriptor(text):
        return "9999 Nobody 127.0.0.1 300 Nobody\n0 Nobody 127.0.0.1 300 Nobody\n" + text

    def no_end_marker(text):
        return "".join(line + "\n" for line in text.split("\n") if line and not line.startswith("-1"))

    for name, mangle in (("truncated", truncated), ("garbage", garbage_first), ("wrong descriptors", wrong_descriptor),
                         ("no end marker", no_end_marker), ("missing", None)):
        print(f" - {name}", flush=True)
        server = Server("damaged")
        check(f"{name}: boots", server.start())
        a, b = server.login(ACCOUNTS[0]), server.login(ACCOUNTS[1])
        path = os.path.join(server.data, "copyover.dat")
        # The new process reads it after booting, so hold the boot to damage it, as a failing disk might.
        server.hold_next_boot()
        a.send("copyover", 0)
        end = time.time() + 10
        while not os.path.exists(path) and time.time() < end:
            time.sleep(0.002)
        with open(path) as f:
            text = f.read()
        if mangle:
            with open(path + ".new", "w") as f:
                f.write(mangle(text))
            os.replace(path + ".new", path)
        else:
            os.remove(path)
        check(f"{name}: the new process was held", server.release_boot())
        up = server.wait_up()
        check(f"{name}: the game keeps running", up and server.process.poll() is None, server.process.poll())
        a.pump(1)
        b.pump(1)
        hanging = [c for c in (a, b) if not c.closed and "Folding complete" not in c.text()]
        check(f"{name}: nobody is left waiting on a dead connection", not hanging, [c.text()[-60:] for c in hanging])
        newcomer = Client(server.port)
        newcomer.pump(0.7)
        check(f"{name}: new players can connect", len(newcomer.text()) > 10)
        server.finish()


def scenario_old_format_file():
    server = Server("oldformat")
    check("boots", server.start())
    a = server.login(ACCOUNTS[0])
    path = os.path.join(server.data, "copyover.dat")
    server.hold_next_boot()
    a.send("copyover", 0)
    end = time.time() + 10
    while not os.path.exists(path) and time.time() < end:
        time.sleep(0.002)
    with open(path) as f:
        text = f.read()
    with open(path + ".new", "w") as f:
        f.write(re.sub(r"-1 restrict \d+", "-1", text))  # what the server before this one wrote
    os.replace(path + ".new", path)
    check("the new process was held", server.release_boot())
    check("comes back", server.wait_up())
    check("player comes back", a.wait_for("Folding complete"))
    server.finish()


def scenario_timed():
    server = Server("timed")
    check("boots", server.start())
    a = server.login(ACCOUNTS[0])
    a.send("copyover 70")
    check("can be cancelled", "cancelled" in a.ask("copyover cancel").lower())
    a.send("copyover 3")
    check("happens on time", server.wait_up())
    check("player comes back", a.wait_for("Folding complete"))
    check("doesn't announce 0 seconds", "in 0 seconds" not in a.text())
    server.finish()


def scenario_deadlock_timer():
    # A server from before this was fixed leaves the timer running across its exec.
    def inherit_a_running_timer():
        signal.setitimer(signal.ITIMER_VIRTUAL, 0.05, 0)

    server = Server("timer")
    check("survives the deadlock timer running out during boot", server.start(before_exec=inherit_a_running_timer),
          f"exit status {server.process.poll()}")
    server.finish()


SCENARIOS = {name[len("scenario_"):]: fn for name, fn in globals().items() if name.startswith("scenario_")}


def main():
    names = sys.argv[2:] or list(SCENARIOS)
    try:
        for name in names:
            print(f"== {name}", flush=True)
            before = len(results)
            try:
                SCENARIOS[name]()
            except Exception as error:
                check(f"{name} ran to the end", False, repr(error)[:200])
            # the same check names repeat between scenarios, so keep them apart
            for key in list(results)[before:]:
                results[f"{name}: {key}"] = results.pop(key)
    finally:
        for server in servers:  # a scenario that stopped early won't have finished its own
            if getattr(server, "process", None) and server.process.poll() is None:
                server.process.kill()
                server.process.wait()
        shutil.rmtree(WORK, ignore_errors=True)
    failed = [name for name, ok in results.items() if not ok]
    print(f"{len(results) - len(failed)}/{len(results)} checks passed")
    for name in failed:
        print(f"  FAILED: {name}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
