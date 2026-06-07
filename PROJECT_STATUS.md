**Branch:** `giulia_safemerge`

> Merge of `server_dev` (socket/poll infrastructure + parser) and `aaron-dev` (IRC command handling + channels).

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

## Next steps
- **Error handling:** `bind`/`listen` failures print an error but the server
  keeps running and enters the poll loop on a bad socket (busy-loop at 100% CPU).
  Should `throw` / stop instead.
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
