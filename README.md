notifier
========
A program that fetches notifications from given sources and displays them on desktop.  
**Note: The program is currently in early stages of development, and breaking changes may occur frequently.**

## Availability
Currently the program works only on Linux.

## Building
Required dependencies:
- GCC
- CMake
- make
- iniparser
- libcurl

Optional dependencies:
- libnotify - for libnotify display
- json-c - for GitHub/ISOD/Twitch modules
- libxml2 - for RSS module

Linux:
```
git clone https://github.com/gardockt/notifier.git
cd notifier
mkdir build
cd build
cmake ..
make
```

## Configuration
Configuration file is an INI file that should be placed in `$HOME/.config/notifier/notifier.ini`. For each module a section has to be made - alongside with fields required by module, it should contain following fields:
- `module` - notifications module to be used
- `display` - display module used for notifying user
- `interval` - interval in seconds between checking for notifications
- `title` - notification's title
- `body` - notification's body
- `icon` - path to icon displayed in notifications (optional)
- `verbosity` - logging verbosity; number from 0 to 3, bigger value means more verbose messages will be printed (optional, default is 0)

Text in `title` and `body` may contain variables, depending on module.

Available notification modules:
- `github` - GitHub notifications
- `isod` - ISOD messages
- `rss` - RSS feed
- `twitch` - Twitch streams

Available display modules:
- `libnotify`

Module configuration details:
- GitHub
	- Required fields:
		- `token` - OAuth token
	- Available variables:
		- `<repo-name>` - repository's name
		- `<repo-full-name>` - repository's full name (containing owner)
		- `<title>` - notification's title
- ISOD
	- Required fields:
		- `max_messages` - max amount of new messages to be displayed
		- `token` - API token
		- `username` - username
	- Available variables:
		- `<title>` - message's title
- RSS
	- Required fields:
		- `sources` - whitespace-separated list of RSS URLs
	- Available variables:
		- `<source-name>` - name of RSS feed
		- `<title>` - message's title
- Twitch
	- Required fields:
		- `id` - app ID used to access Twitch API
		- `secret` - OAuth secret for refreshing the token
		- `streams` - whitespace-separated list containing nicknames of streamers to be checked
	- Available variables:
		- `<category>` - streamed game/category
		- `<title>` - stream's title
		- `<streamer-name>` - streamer's nickname

Configuration file can also contain a section `_global`, consisting of default values for each field. Alternatively, there can also be a section named `_global.[module_type]`, which values will be applied only to modules of given type.

Example configuration file:
```
[_global]
interval = 120
body = <title>
verbosity = 1

[_global.github]
title = <repo-name>

[module]
module = twitch
title = <streamer-name> is streaming <category>!
id = ENTER_YOUR_ID_HERE
secret = ENTER_YOUR_SECRET_HERE
streams = gamesdonequick twitchplayspokemon
interval = 30
icon = /usr/share/local/icons/twitch.png

[other_module]
module = github
token = ghp_enteryourtokenhere
icon = /usr/share/local/icons/github.png

[and_another_one]
module = isod
title = ISOD
username = enter_your_username_here
token = ENTER_YOUR_TOKEN_HERE
max_messages = 4
icon = /usr/share/local/icons/isod.png

[last_one]
module = github
token = ghp_anothertoken
```
