# eq-dps-girl

An EQ damage meter (only tested with THJ at the moment, but would
probably work on any EQ server).

![eq-dps-girl](https://github.com/ahungry/eq-dps-girl/blob/master/eq-dps-girl.gif)

# Usage

Using the design philosophy of a FF14 dps/hud (shown here):

https://www.iinact.com/

the parser runs as a cross-platform web server that is available on
http://localhost:12345, while the overlay functionality is available
via one of the following:

- GNU/Linux: https://github.com/anko/hudkit
- Windows: https://github.com/Styr1x/Browsingway
- Mac/OSX: https://www.iinact.com/bunnyhud/

The web server does require Babashka (a Clojure runtime):

https://github.com/babashka/babashka

NOTE: I only use GNU/Linux, so I've only tested under that environment.

If you've got Babashka (bb) installed, launching is as easy as this
(on GNU/Linux - other OS scripts like powershell coming soon):

```sh
./server.bb & # it has a shebang for bb so is executable directly
hudkit http://localhost:12345 # make sure you have a compositor!
```

NOTE: If you don't have a compositor running, `xcompmgr` is a super
light weight one that doesn't cause VSync issues on the THJ EQ client
(though it does on Diablo 4).

# TODO

- Make user options easier to configure (without having to edit the
server.bb file) - likely via user input on http://localhost:12345 for
actions such as selecting the EQ log directory and setting other
values and optional features (like sleep idle time, auto log
truncation etc.).
- Add launch scripts to auto-pull Babashka per host environment

# License

GPLv3 or later

Distributed under the GNU General Public License either version 3.0 or (at
your option) any later version.
