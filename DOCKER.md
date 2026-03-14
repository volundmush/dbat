# Docker Development Environment (`dbat`)

This project includes a Docker Compose-based development environment so you can build on **Ubuntu 24.04** with a consistent toolchain.

The setup is designed to:

- match your target distro baseline (`ubuntu:24.04`)
- use modern C/C++ tooling (`clang`, `cmake`, `ninja`)
- provide system dependencies required by `dbat` and `volcano` (when brought in via CPM)

---

## What this gives you

A containerized dev shell with these packages available:

- `clang`
- `cmake`
- `ninja-build`
- `pkg-config`
- `git`
- `build-essential`
- `libssl-dev` (OpenSSL)
- `libboost-all-dev` (Boost)
- `zlib1g-dev` (ZLIB)
- `liburing-dev`
- `libbsd-dev`
- `ca-certificates`

`Threads` support is provided by the standard C/C++ toolchain (`pthread` / glibc).

---

## Expected files

This doc assumes these files exist in the repo:

- `compose.yml`
- `docker/Dockerfile`
- `scripts/container-shell.sh`
- `scripts/container-build.sh`
- `.env.example` (optional helper)

---

## Quick start

From the repo root (`dbat`):

1. Build the dev image:
   ```bash
   docker compose build
   ```

2. Open an interactive shell in the container:
   ```bash
   docker compose run --rm dev
   ```

3. Configure and build manually inside the container:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
   cmake --build build -j
   ```

Or run the scripted build directly:

```bash
./scripts/container-build.sh
```

---

## Typical workflow

### Open a shell
```bash
./scripts/container-shell.sh
```

### Build
```bash
./scripts/container-build.sh
```

### Rebuild from scratch
```bash
rm -rf build
./scripts/container-build.sh
```

---

## Notes about CPM dependencies

`dbat` uses CPM to fetch some dependencies (including `volcano` in your current flow).  
For this to work reliably in the container:

- ensure network access is available
- ensure Git is installed (it is, in the recommended Dockerfile)
- optionally cache CPM downloads between runs to speed up rebuilds

If you want persistent CPM cache, mount a cache directory in `compose.yml` (example):

```yaml
volumes:
  - ./:/workspace/dbat
  - cpm-cache:/workspace/.cache/cpm

volumes:
  cpm-cache:
```

And set an env var used by your CMake/CPM setup (if applicable), for example:

```bash
CPM_SOURCE_CACHE=/workspace/.cache/cpm
```

---

## Environment variables (optional)

You may set these in `.env` for compose:

- `UID` / `GID` (if your compose file maps container user to host user)
- custom cache paths
- extra CMake flags

Start from `.env.example` if provided:

```bash
cp .env.example .env
```

---

## Troubleshooting

### `docker: command not found`
Install Docker (or compatible container runtime with Compose support).

### `Cannot connect to the Docker daemon`
Start/enable the Docker daemon and ensure your user has permission.

### File ownership issues on generated build files
Run container with host UID/GID mapping in `compose.yml`, or clean ownership with:
```bash
sudo chown -R $USER:$USER build
```

### CPM downloads fail
Check network/proxy settings and Git access to dependency repos.

### CMake cannot find libraries
Verify system packages are installed in the image and that you are using the project container (not host shell).

---

## Recommended conventions

- Use out-of-source builds (`build/`)
- Keep compiler explicit (`clang`/`clang++`)
- Prefer `Ninja` generator
- Keep container config versioned in git
- Rebuild image after changing dependencies:
  ```bash
  docker compose build --no-cache
  ```

---

## One-command habits

Once scaffolded, these should be your defaults:

```bash
# start dev shell
./scripts/container-shell.sh

# build project
./scripts/container-build.sh
```

This keeps your host clean and your build environment reproducible against Ubuntu 24.04.