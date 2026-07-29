# Top Demons

Top Demons brings a verified Top 150 of rated Extreme Demons directly into
Geometry Dash.

## Features

- Adds a large **Top 150** button with an Extreme Demon face to the Creator
  menu.
- Keeps ranking positions 1 through 150 in their original order.
- Verifies each Geometry Dash ID and accepts only rated Extreme Demons.
- Replaces an invalid ID with the valid rated level that has the same name.
- Supports search by level name or Geometry Dash level ID.
- Includes Top 75, Top 150, custom-range, rating, and creator filters.
- Supports paging, random-page navigation, refresh, and normal level pages.

## How to use

1. Install `mbmb709.topdemons.geode` and restart Geometry Dash.
2. Open the Creator menu from the button next to Play.
3. Press **Top 150**, marked with the Extreme Demon face.
4. Browse, search, or filter the list.
5. Select a result to open its normal Geometry Dash level page.

An internet connection is required when ranking or level information is not
already cached.

## Supported platforms

- Windows 64-bit
- Android 32-bit
- Android 64-bit
- macOS, Intel and Apple Silicon
- iOS and iPadOS ARM64

The mod targets Geometry Dash 2.2081 and Geode 5.8.2.

## Building

The included GitHub Actions workflow builds all supported targets and combines
them into one `mbmb709.topdemons.geode` package. You can also build the current
platform locally with:

```sh
geode build
```

## Credits and license

Top Demons contains adapted code from
[Global List Integration](https://github.com/XBOCTATUK/Global-List-Integrated)
by XBOCTATUK, used under the MIT License. The original copyright notice is
preserved in [LICENSE](LICENSE).
