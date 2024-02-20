<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]



<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/reidprichard/repo_name">
    <img src="images/logo.png" alt="Logo" width="80" height="80">
  </a>

<h3 align="center">Window Tools</h3>

  <p align="center">
    This project consists of two simple tools for interacting with windows. The first, window_manager, allows you to save (and later refocus) any window with a single command. The second communicates with Kanata's TCP server to set application-specific keybinds.
    <br />
    <a href="https://github.com/reidprichard/repo_name"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/reidprichard/repo_name">View Demo</a>
    ·
    <a href="https://github.com/reidprichard/repo_name/issues">Report Bug</a>
    ·
    <a href="https://github.com/reidprichard/repo_name/issues">Request Feature</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

[![Product Name Screen Shot][product-screenshot]](https://example.com)

`This project consists of two simple tools for interacting with windows. The
first, window_manager, allows you to save (and later refocus) any window with a
single command. The second communicates with Kanata's TCP server to set
application-specific keybinds.`

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

### Installation

1. Download the project executables from the latest release.
2. Run via the command line.
3. That's it! No config file needed.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

### kanata_helper_daemon
To get started, all you need to do is run:
`.../path/to/kanata_helper_daemon.exe --port=<kanata-port-number>`

For example, if you had launched Kanata with:
`kanata.exe -p 1337`

You would run:
`.../path/to/kanata_helper_daemon.exe --port=1337`

When you switch your application focus, kanata_helper_daemon will send a message to Kanata telling it to switch layers.
The requested layer name is the name of the window's process. For example, if you opened Firefox, kanata_helper_daemon
would attempt to switch to the layer named "firefox". If you're not sure what a window's process name is, take a look
at kanata_helper_daemon's console output. Note that layer names are case-sensitive.

For full functionality, you'll want to point kanata_helper_daemon to your Kanata config file, like so: . 
<pre>
.../path/to/kanata_helper_daemon.exe --port=1337 <u>--config-file=.../path/to/config/file.kbd</u>
</pre>
The daemon will parse your config file to gather a list of all your layer names. If a process
name does not match one of your layer names, it will return to the layer named "default".
If you don't have a layer named "default", or you just want to use a different layer for this,
you can specify the name like so:
<pre>
.../path/to/kanata_helper_daemon.exe --port=1337 --config-file=.../path/to/config/file.kbd <u>--default-layer=my-default-layer</u>
</pre>

You will likely want to run this daemon in the background so that you don't need to keep a
console window open. To do so, you can use the following syntax in PowerShell:
<pre>
<u>Start-Process</u> .../path/to/kanata_helper_daemon.exe --port=1337 --config-file=.../path/to/config/file.kbd --default-layer=my-default-layer <u>-WindowStyle Hidden</u>
</pre>

If you wish to kill this background process, you can do so (again in PowerShell) with:
`Stop-Process -Name kanata_helper_daemon`

### window_manager

This tool allows you to programatically activate specific windows. Do you often find yourself
searching through a million icons on your taskbar to find that one browser window? This is the
tool for you.

Usage is relatively straightforward. The command:
`/path/to/window_manager.exe --save-window=0`
saves the currently-focused window to index 0, and:
`/path/to/window_manager.exe --load-window=0`
will later bring that window to the front and focus it. The window's index can be
any number from 0-999. 

Saved windows are stored in a .ini file. By default, this will be "saved_windows-<computer-name>.ini".
If you wish to change this file, you can do so with the `--path` argument:
<pre>
`/path/to/window_manager.exe <u>--path=.../path/to/your/file.ini</u> --save-window=0`
</pre>

Both window title and handle (basically, a number that is a unique identifier for that particular window) are stored. 
This allows window_manager to activate a window after it has been closed and reopened (handle changed,
but title hopefully remained the same) or if the window title changes (e.g. you change tabs in your browser).
Since this utility is run on-demand, and there is no background daemon, it is limited in what it can
do to keep track of your windows. Each time you activate a window it will update the saved handle and title,
but if a window's handle _and_ title change between activations window_manager won't know where to find the
window.

Obviously, running this utility directly from the terminal isn't very useful. It is intended
to be bound to your keyboard. You can see a Kanata configuration that does this in `examples`.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ROADMAP -->
## Roadmap

- [ ] Add system tray icon for kanata_helper_daemon background execution
- [ ] Hook kanata_helper_daemon to Win32 events rather than continually looping
- [ ] Add background service for window_manager to update handles and titles as they change. This could be handled by kanata_helper_daemon.

See the [open issues](https://github.com/reidprichard/repo_name/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- LICENSE -->
## License

Distributed under the GNU General Public License v3.0 or later. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Project Link: [https://github.com/reidprichard/repo_name](https://github.com/github_username/repo_name)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/reidprichard/repo_name.svg?style=for-the-badge
[contributors-url]: https://github.com/reidprichard/repo_name/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/reidprichard/repo_name.svg?style=for-the-badge
[forks-url]: https://github.com/reidprichard/repo_name/network/members
[stars-shield]: https://img.shields.io/github/stars/reidprichard/repo_name.svg?style=for-the-badge
[stars-url]: https://github.com/reidprichard/repo_name/stargazers
[issues-shield]: https://img.shields.io/github/issues/reidprichard/repo_name.svg?style=for-the-badge
[issues-url]: https://github.com/reidprichard/repo_name/issues
[license-shield]: https://img.shields.io/github/license/reidprichard/repo_name.svg?style=for-the-badge
[license-url]: https://github.com/reidprichard/repo_name/blob/master/LICENSE.txt
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/linkedin_username
[product-screenshot]: images/screenshot.png
[Next.js]: https://img.shields.io/badge/next.js-000000?style=for-the-badge&logo=nextdotjs&logoColor=white
[Next-url]: https://nextjs.org/
[React.js]: https://img.shields.io/badge/React-20232A?style=for-the-badge&logo=react&logoColor=61DAFB
[React-url]: https://reactjs.org/
[Vue.js]: https://img.shields.io/badge/Vue.js-35495E?style=for-the-badge&logo=vuedotjs&logoColor=4FC08D
[Vue-url]: https://vuejs.org/
[Angular.io]: https://img.shields.io/badge/Angular-DD0031?style=for-the-badge&logo=angular&logoColor=white
[Angular-url]: https://angular.io/
[Svelte.dev]: https://img.shields.io/badge/Svelte-4A4A55?style=for-the-badge&logo=svelte&logoColor=FF3E00
[Svelte-url]: https://svelte.dev/
[Laravel.com]: https://img.shields.io/badge/Laravel-FF2D20?style=for-the-badge&logo=laravel&logoColor=white
[Laravel-url]: https://laravel.com
[Bootstrap.com]: https://img.shields.io/badge/Bootstrap-563D7C?style=for-the-badge&logo=bootstrap&logoColor=white
[Bootstrap-url]: https://getbootstrap.com
[JQuery.com]: https://img.shields.io/badge/jQuery-0769AD?style=for-the-badge&logo=jquery&logoColor=white
[JQuery-url]: https://jquery.com 

