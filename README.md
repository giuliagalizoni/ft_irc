*This project has been created as part of the 42 curriculum by ggalizon, apaz-mar.*

## Description

ft_irc is a lightweight IRC (Internet Relay Chat) server written in C++98. The goal is to implement the core of the IRC protocol from scratch: handling multiple simultaneous client connections without forking, using non-blocking I/O with `poll()`, and supporting real IRC clients such as irssi.

The server supports full client registration (`PASS`, `NICK`, `USER`), channel management (`JOIN`, `PART`, `TOPIC`, `KICK`, `INVITE`, `MODE`), private messaging, and the standard channel modes (`+i`, `+t`, `+k`, `+l`, `+o`).

## Instructions

**Compilation**
```
make
```
Requires a C++98-compatible compiler (`c++`). Produces the binary `ircserv`.

**Execution**
```
./ircserv <port> <password>
```
- `port`: a valid TCP port (1–65535)
- `password`: the connection password clients must supply

**Connecting with irssi**
```
irssi
/connect 127.0.0.1 <port> <password>
/nick <yournick>
/join #channel
```

**Cleanup**
```
make fclean   # removes binary and object files
```

## Resources

- [RFC 1459 – Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 – IRC: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [The Serial Port - Internet Relay Chat](https://www.youtube.com/watch?v=6UbKenFipjo)
- [Network Chuck - What is the TCP/IP and OSI?](https://www.youtube.com/watch?v=CRdL1PcherM)
- [Creating a TCP Server in C++](https://www.youtube.com/watch?v=cNdlrbZSkyQ&list=PLHBVNH27RbWqGTL-AYMylWkNck45cxPnG)
- `man poll`, `man socket`, `man recv`, `man send`, `man fcntl`

**AI usage:**

AI was used following the 42 guidelines: for general research and supporting the learning process, as well to debug and understand problems. The core logic and design of the project was implemented by the authors.
AI was used to:
- Facilitate the understanding of core concepts of the IRC protocol and networking;
- Understand the relations between IRC protocol, server and client;
- Debug and troubleshoot issues;
- Review code cleaness and style;
- Execute repetitive tasks (code formatting, repetitive code for error handling);
