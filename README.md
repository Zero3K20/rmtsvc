rmtsvc
==========
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Environment](https://img.shields.io/badge/Windows-XP,%20Vista,%207,%208,%2010-yellow.svg)
![License](https://img.shields.io/github/license/hsluoyz/rmtsvc.svg)

rmtsvc is the abbreviation for **ReMoTe SerViCe**. It is a web-based remote desktop &amp; control service for Windows. [yycnet](http://bbs.pediy.com/member.php?u=106711) open-sourced the 2.5.2 code of rmtsvc at http://bbs.pediy.com/showthread.php?t=184683. The current version is **v3.0**.

rmtsvc supports **Windows XP and later (including Win7, Win8 and Win10)**. You can use your web browser to remotely control a Windows machine, including **remote desktop, command execution, process monitoring**, etc.

## Build

* Open `rmtsvc.sln` and build using **Visual Studio 2019** (or later), you should get the binary: `rmtsvc.exe`.

## Run

1. Put `example\rmtsvc.ini` in the same folder with `rmtsvc.exe`.
2. Double-click `rmtsvc.exe` to start it (a console window will appear). Alternatively, run `rmtsvc.exe -h` to start it with the console window hidden.
3. Launch your web browser to `http://127.0.0.1:777`, username is `abc`, password is `123`, log on the portal and do your stuff.

## Auto-start at login

To have rmtsvc start automatically when you log in to Windows, run:

```
rmtsvc.exe -i
```

This adds a startup registry entry (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`) so rmtsvc launches hidden (no console window) on login. To remove the startup entry, run:

```
rmtsvc.exe -u
```

## Download

If you don't want to build the binary by yourself, the prebuilt binaries can always be found here:
https://github.com/Zero3K20/rmtsvc/releases

## License

rmtsvc is published under [**The MIT License (MIT)**](http://opensource.org/licenses/MIT).

## Contact

* hsluoyz@gmail.com
