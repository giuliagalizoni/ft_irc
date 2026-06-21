**Branch:** `giulia-signal-handling`

> Branched from `giulia_safemerge` (merge of `server_dev` socket/poll infrastructure +
> parser, and `aaron-dev` IRC command handling + channels), building on the
> `exceptions_handling` work below.
> This branch adds **signal handling and graceful shutdown** on top of the existing
> error handling, exceptions, and argument validation.

## What's working
- TCP socket bound with `INADDR_ANY`, listening; port + password taken from `argv`
- `poll()` loop over a dynamic `std::vector<pollfd>` — handles multiple clients
- Accepts connections with `accept()`; I/O made non-blocking per-call via `MSG_DONTWAIT` on `recv`/`send` (no `fcntl`/`accept4` — not in the allowed list on Linux)
- Reads with `recv`, buffers per-client, extracts complete lines

## Command processing
- Generic command parser (`_parseCommand` → `getCommand` + `getParams`)
- Parser handles the **trailing parameter** (`:`) correctly, so multi-word
  arguments arrive as one param (e.g. realname, channel message)
- Commands implemented: PASS, NICK, USER, JOIN, PRIVMSG, PART, TOPIC, QUIT, INVITE, KICK
- The old `Message` struct was dropped; we kept the simpler `Command` struct
  (no `prefix` field — prefixes are only for server-to-server, which we don't do)

## Line endings
- `getNextLine` accepts **both** `\r\n` (real IRC clients) and bare `\n`
  (raw `nc`/`telnet`): it splits on `\n` and strips a trailing `\r` if present

## Registration
- Password validation against server password
- Registration completed when PASS + NICK + USER have been received
- Registration announcement triggered only once

## Channels
- Channel creation on first JOIN
- Operator assigned to first user joining the channel
- JOIN notifications broadcast to all channel members
- PART removes a user from a channel and deletes the channel if empty
- TOPIC can display and update a channel topic
- INVITE stores invited users for invite-only channel support
- KICK removes a target user from a channel after operator validation

## Messaging
- Channel messages: `PRIVMSG #channel :message`
- Direct messages: `PRIVMSG nickname :message`
- Sender excluded from broadcast
- Nickname uniqueness validation implemented

## Error handling & exceptions (this branch)
Split into three categories: **fatal startup errors** (C++ exceptions), **recoverable
runtime errors** (handled in-loop, never throw), and IRC protocol errors (numeric
replies — *not yet done, future work*).

**Phase 1 — fatal startup errors → exceptions**
- New `SetupException : public std::exception` (`includes/SetupException.hpp` +
  `src/SetupException.cpp`); declaration in header, bodies in `.cpp` (no impl in
  headers); `what() const throw()` returns a stored message.
- `Server` constructor now **throws** `SetupException` on `socket`/`setsockopt`/
  `bind`/`listen` failure instead of printing and falling through. Close-then-throw
  at every site after the socket exists (no fd leak; `~Server()` doesn't run on a
  ctor throw). This fixes the old **100% CPU busy-loop** on a bad socket.
- `main` wraps construction + `run()` in `try/catch (const std::exception&)`, prints
  to `stderr`, returns **non-zero**.

**Phase 1d — argument validation (in `main`, before constructing `Server`)**
- `validPort` uses `std::istringstream` (not `atoi`): rejects empty, junk
  (`6667abc`, `66x7`), leading `+`, overflow, and out-of-range; accepts **1–65535**
  (privileged ports <1024 left to fail at `bind()`).
- `validPass` rejects empty + any whitespace/control char (so the password is always
  matchable via `PASS`, which the parser splits on spaces).
- Bad args → message to `stderr` + non-zero return (no exception — no server yet).

**Phase 2 — recoverable runtime errors → no throw**
- `poll() == -1`: now branches on `errno` — **`EINTR`** (signal-interrupted) `continue`s
  to re-check `g_running` (clean shutdown), any **other** errno prints once and `break`s.
  (See the signal-handling section below.)
- `recv() <= 0`: treat as disconnect (no `errno` check) — see deferred-disconnect note below.
- `run()` now also reacts to `POLLHUP`/`POLLERR`/`POLLNVAL`, not just `POLLIN`, so
  dead/errored sockets are cleaned up instead of spinning.
- **Verified:** valgrind across connect/disconnect cycles → `definitely lost: 0`,
  `0 errors`. The old "still reachable" on `SIGINT` is now resolved — SIGINT is caught,
  so `~Server()` runs and frees everything (see signal-handling section below).

## Deferred client disconnect (latest)
**What:** disconnects no longer happen inline. Each `User` carries a `_disconnecting`
flag; the recv-disconnect path, `_handleQuit`, and `sendMessage` (on a real send error,
not `EAGAIN`) now just **mark** the user. After each `poll` cycle, `run()` sweeps `_users`
(two-pass: collect fds → then delete) and a single `_disconnectClient(fd)` owns all
teardown: erase from `_fds`, `close`, `delete`, `erase` from `_users`.

**Why:** the old code deleted a `User` deep in the call stack while the poll loop / recv
buffer loop still held that pointer — a **use-after-free on QUIT**, plus the QUIT path
left the closed fd in `_fds`, causing a `POLLNVAL` **CPU spin**. Centralizing teardown in
one place, run once per cycle, removes both. `sendMessage` no longer throws/`cerr`-aborts
on a dead client — a broken pipe drops that one client, not the server. Verified: QUIT +
reconnect keeps the server alive at ~0% CPU, one clean `User deleted` per client.

## Signal handling & graceful shutdown (this branch)
- New `src/signals.cpp` + `includes/signals.hpp`: a single global flag
  `volatile sig_atomic_t g_running` (declared `extern` in the header, defined in the
  `.cpp`), a `static` async-signal-safe handler that only sets `g_running = 0`, and a
  `setupSignals()` registration function.
- Registration via `sigaction()` with `sa_flags = 0` (no `SA_RESTART`): **SIGINT,
  SIGTERM, SIGQUIT** caught (graceful shutdown), **SIGPIPE** set to `SIG_IGN` so a
  write to a disconnected client returns `EPIPE` instead of killing the server. Each
  `sigaction` call is error-checked and throws `SetupException` on failure.
- `main()` calls `setupSignals()` inside the existing `try`, before constructing `Server`.
- `Server::run()` loop is now `while (g_running)`; the `poll() == -1` branch is
  **`EINTR`-aware** — a signal-interrupted `poll` is not an error, it just `continue`s
  and lets the loop condition fall through to clean shutdown (real errors still break).
- `~Server()` now sends `ERROR :Server is shutting down\r\n` to every connected user
  **before** closing sockets (best-effort; safe because SIGPIPE is ignored), then runs
  the existing close-fds + delete-users cleanup.
- **Not implemented (by design):** server-stdin Ctrl+D shutdown — not required by the
  subject (its Ctrl+D test is client-side packet aggregation, already handled by
  `getNextLine`), and risky if stdin is closed/redirected. SIGTSTP (Ctrl+Z) left as
  default job-control suspend.

## Next steps
- **IRC protocol errors (numerics):** replace `std::cout` "errors" with real numeric
  replies (`461`, `433`, `403`, `401`, `464`, `451`, `421`) sent to the client.
- **Registration gating:** reject commands before registration with `451`.
- **Message prefix:** outgoing PRIVMSG/JOIN/PART/QUIT/TOPIC/INVITE/KICK use `:nick` only; RFC form is the
  full `:nick!user@host` (affects JOIN, PART, QUIT, KICK too)
- Remove leftover debug `std::cout` lines before submission
- More commands: MODE
- Test with IRC client irssi

## How to run
```bash
make
./ircserv 6667 pass
```
In another terminal (plain `nc` works now that bare `\n` is accepted):
```bash
nc localhost 6667
```

## Tests
Registration
    PASS pass
    NICK charlie
    USER charlie 0 * :Charles Chaplin

Channel join
    JOIN #test

Channel message
    PRIVMSG #test :hello everyone

Direct message
    PRIVMSG bob :hello privately
