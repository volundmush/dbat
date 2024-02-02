#!/usr/bin/env python

def run(name: str):
    import os
    import dbat
    from dbat.utils import import_from_module

    from dbat import settings

    core_class = import_from_module(settings.CORES[name])

    pidfile = f"{name}.pid"

    with open(pidfile, "w") as pid_f:
        pid_f.write(str(os.getpid()))
        pid_f.flush()

        try:
            app = core_class(settings)
            dbat.GAME = app
            app.run()
        except Exception as err:
            print(str(err))
    os.remove(pidfile)


if __name__ == "__main__":
    run("portal")
