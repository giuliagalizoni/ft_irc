**Branch:** `exceptions_handling`

> Branched from `giulia_safemerge` (merge of `server_dev` socket/poll infrastructure +
> parser, and `aaron-dev` IRC command handling + channels).
> This branch adds **error handling, exceptions, and argument validation**.

## What's working
- TCP socket bound with `INADDR_ANY`, listening; port + password taken from `argv`
- `poll()` loop over a dynamic `std::vector<pollfd>` — handles multiple clients
- Accepts connections with `accept4()` (non-blocking)
- Reads with `recv`, buffers per-client, extracts complete lines

## Command processing
- Generic command parser (`_parseCommand` → `getCommand` + `getParams`)
- Parser handles the **trailing parameter** (`:`) correctly, so multi-word
  arguments arrive as one param (e.g. realname, channel message)
- Commands implemented: PASS, NICK, USER, JOIN, PRIVMSG
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
- `poll() == -1`: print once and **`break`** out of the loop (clean shutdown via
  destructor) instead of `continue` (which busy-looped).
- `recv() <= 0`: treat as disconnect (no `errno` check) — `find(fd)` → `delete` user
  → `erase` → `close`; `run()` removes the pollfd.
- `run()` now also reacts to `POLLHUP`/`POLLERR`/`POLLNVAL`, not just `POLLIN`, so
  dead/errored sockets are cleaned up instead of spinning.
- **Verified:** valgrind across connect/disconnect cycles → `definitely lost: 0`,
  `0 errors`. (Remaining "still reachable" only appears when the server is
  `SIGINT`-killed mid-run, since `~Server()` doesn't run then — see Next steps.)

## Next steps
- **Graceful shutdown:** add a signal handler so Ctrl-C makes `run()` break and
  `~Server()` runs — needed for a fully clean valgrind report (no "still reachable").
  Pairs with handling `EINTR` on `poll()` as a retry instead of exit.
- **IRC protocol errors (numerics):** replace `std::cout` "errors" with real numeric
  replies (`461`, `433`, `403`, `401`, `464`, `451`, `421`) sent to the client.
- **Registration gating:** reject commands before registration with `451`.
- **Message prefix:** outgoing PRIVMSG/JOIN use `:nick` only; RFC form is the
  full `:nick!user@host` (affects JOIN, PART, QUIT, KICK too)
- Remove leftover debug `std::cout` lines before submission
- More commands: MODE, TOPIC, PART, QUIT, KICK, INVITE

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
