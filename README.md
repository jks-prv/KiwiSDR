[updated 26 June 2026]

[![KiwiSDR](http://www.kiwisdr.com/ks/kiwi2.780px.jpg)](http://www.kiwisdr.com/ks/kiwi2.1024px.jpg)

Click image for full size.

[![KiwiSDR](http://www.kiwisdr.com/ks/kiwi-with-headphones.130x170.png)](http://kiwisdr.com)

KiwiSDR
=======

> [!IMPORTANT]
> This is a continuation of the previous repo named "Beagle_SDR_GPS".

Software-defined Radio (SDR) and GPS for the BeagleBone
----------------------------------------------------------------------

An add-on board ("cape") that turns your BeagleBone into a web-accessible shortwave receiver.

### Details

* Listen live: [List](http://rx.kiwisdr.com), [Map](http://map.kiwisdr.com)
* Listening apps:
    * [AM_SW Radio Online Streamer (iOS)](https://apps.apple.com/app/am-sw-radio/id6759920170)
    * [Echo: The Universal iOS SDR Client](https://apps.apple.com/app/echo-global-sdr-receiver/id6759174390)
    * [QiwiQ: a KiwiSDR client for Android](https://vanbarel.eu.org/tools/qiwiq)
* [Online store](https://kiwisdr.nz)
* [Project webpage](http://www.kiwisdr.com)
* [Operating information: installation, operation, FAQ](http://www.kiwisdr.com/info)
* [User forum](http://forum.kiwisdr.com) (signup required)
* Latest [source code commits](https://github.com/jks-prv/KiwiSDR/commits/master)
* Previous [source code commits](https://github.com/jks-prv/Beagle_SDR_GPS/commits/master)
* [Design review document](http://kiwisdr.com/docs/KiwiSDR/KiwiSDR.design.review.pdf)
* [KiwiSDR 1 schematic](http://www.kiwisdr.com/docs/KiwiSDR/kiwi.schematic.pdf)
* [KiwiSDR 2 schematic](http://www.kiwisdr.com/docs/KiwiSDR/kiwi-2.schematic.pdf)
* [KiwiSDR 2 rev 1.1 schematic](http://www.kiwisdr.com/docs/KiwiSDR/kiwi-2-rev1.1.schematic.pdf)

### Description
This SDR is a bit different. It has a web interface that can be used by up to four separate listeners. Each one listening and tuning an independent frequency simultaneously. See the screenshots below.

### Components
* SDR covering the 10 kHz to 30 MHz (VLF-HF) spectrum.
* Web interface based on [OpenWebRX](https://github.com/ha7ilm/openwebrx) from András Retzler, HA7ILM.
* Integrated software-defined GPS receiver from Andrew Holme's [Homemade GPS Receiver](http://www.aholme.co.uk/GPS/Main.htm).
* LTC/Analog Devices 14-bit 65 MHz ADC.
* Xilinx/AMD Artix-7 A35 FPGA.
* Maxim/Analog Devices MAX2769B GPS front-end.
* Kiwi board works with BeagleBone Green, Green Eco, Black, BBAI or BBAI-64.

### Features
* Open Source.
* Browser-based interface allowing multiple simultaneous user web connections.
* Each connection tunes an independent receiver channel over the entire spectrum.
* Waterfall tunes independently of audio and includes zooming and panning.
* Multi-channel, parallel DDC design using bit-width optimized CIC filters.
* Built-in signal decoding: ALE CW DRM FAX FSK FT8 HFDL IBP NAVTEX/DSC SSTV TDoA WSPR and more.
* Good performance at VLF/LF since we personally spend time monitoring those frequencies.
* Automatic frequency calibration via received GPS timing.
* Easy hardware and software setup. Browser-based configuration interface.
* KiwiSDR 2 features:
    * Reverse proxy service enabled by default to ease network installation.
    * External ADC clock input.
    * Built-in self test for checking the RF path.
    * 5V input reverse polarity protection.

### Status

Give the live receivers a try at the links above. The web interface works, with some problems, on mobile devices. But there is no mobile version of the interface yet. The mobile apps listed above provide good alternatives.

A second generation device, the [KiwiSDR 2](https://forum.kiwisdr.com/index.php?p=/discussion/2986/kiwisdr-2-prototypes-working/p1), is now in production.

### Objectives

We wanted to design an SDR that provides certain features that we felt weren't covered by current devices. The SDR must be web-accessible and simple to setup and use.
We also wanted to provide a self-contained platform for experimentation with SDR and GPS techniques. The TDoA extension is an example.

Most importantly, we wanted to see a significant number of web-enabled, wide-band SDRs deployed in diverse locations world-wide because that makes possible some really interesting applications and experiments. Over 800 Kiwis are [publicly available](http://rx.kiwisdr.com) currently.

### Hardware

The KiwiSDR 2 consists of the Kiwi board and BeagleBone Green (software pre-installed) in a metal case, with GPS antenna, recovery sd card and a self-test cable (see [kiwisdr.com](http://www.kiwisdr.com/)). An optional external SDR protection circuit and MW band filter are available.

### Operation

An antenna solution must be provided. An adequate power supply (e.g. 5V @ 2A) will also be required. And of course a wired Ethernet network connection. Inexpensive Ethernet-to-WiFi adapters can also be used.

A reverse proxy service is enabled by default. So the Kiwi is immediately available at [(serial_number).proxy.kiwisdr.com](http://serial_number.proxy.kiwisdr.com). Other network configuration modes are available. Such as using a user-owned domain name, DDNS/DUC, fixed IP address etc. Of course the system can be configured to only allow connections from the local network and ignore Internet connection requests.

### Web interface and built-in signal decoder screenshots:

Click images for full size.

#### Not many SDRs can show the entire 10 kHz to 30 MHz spectrum at one time.

[![](http://www.kiwisdr.com/README/full.780px.png)](http://www.kiwisdr.com/README/full.png)

#### Waterfall/spectrum has 15 levels of zoom (z0 - z14).
Here is z14 (2 kHz span) on the left showing the 25 Hz sweep rate of an Over the Horizon Radar (OTHR) which is very helpful with signal identification. On the right is z10 showing the full passband.

[![](http://www.kiwisdr.com/README/z14.OTHR.780px.png)](http://www.kiwisdr.com/README/z14.OTHR.png)

#### The Kiwi is excellent at VLF/LF reception.
Especially with the right antenna and after eliminating noise sources. When you zoom in further the labels below the spectrum clearly identify all these signals.

[![](http://www.kiwisdr.com/README/VLF_LF.780px.png)](http://www.kiwisdr.com/README/VLF_LF.png)

#### High frequency trading signal on left, CQWW RTTY DX contest on right.

[![](http://www.kiwisdr.com/README/HFT.780px.jpg)](http://www.kiwisdr.com/README/HFT.jpg)

#### The Kiwi has built-in decoders for various ham radio and shortwave signals.
Here is ham slow-scan television decoding.

[![](http://www.kiwisdr.com/README/SSTV.780px.png)](http://www.kiwisdr.com/README/SSTV.png)

#### Ham FT8 mode on 30 meters.
Note FT8 pileup 3 kHz below working YJ0TT Vanuatu.

[![](http://www.kiwisdr.com/README/FT8.30m.780px.png)](http://www.kiwisdr.com/README/FT8.30m.png)

#### Digital Radio Mondiale (DRM), including image slideshow and Journaline text decoding.

[![](http://www.kiwisdr.com/README/DRM.780px.png)](http://www.kiwisdr.com/README/DRM.png)

#### Time Difference of Arrival (TDoA) signal direction finding.
Multiple Kiwis, assisted by their built-in GPS for accurate timing, can cooperate to approximately locate signals.
LF time station DCF77 77.5 kHz in Germany accurately located.

[![](http://www.kiwisdr.com/README/TDoA.DCF77.780px.png)](http://www.kiwisdr.com/README/TDoA.DCF77.png)

#### High Frequency Data Link (HFDL) decoding.
Aircraft to ground station (green) data exchange system. Includes message decoding and aircraft positions (blue) on a map.

[![](http://www.kiwisdr.com/README/HFDL.780px.png)](http://www.kiwisdr.com/README/HFDL.png)

#### Facimile (FAX) decoding.

[![](http://www.kiwisdr.com/README/FAX.780px.png)](http://www.kiwisdr.com/README/FAX.png)

#### Frequency shift keying (FSK, RTTY) decoding.

[![](http://www.kiwisdr.com/README/FSK.780px.png)](http://www.kiwisdr.com/README/FSK.png)

#### Other decoders:

IQ display showing the QPSK modulation of VLF station NLM4 North Dakota (25.2 kHz) as received in Kansas.

[![](http://www.kiwisdr.com/README/IQ.780px.png)](http://www.kiwisdr.com/README/IQ.png)

Decoding of time station WWVB (60 kHz, phase modulation) in Colorado.

[![](http://www.kiwisdr.com/README/timecode.780px.png)](http://www.kiwisdr.com/README/timecode.png)

When the Russian VLF Alpha navigation system is active there's a special decoder for that too.

[![](http://www.kiwisdr.com/README/Alpha.780px.png)](http://www.kiwisdr.com/README/Alpha.png)

A simple monitor for the Loran-C and under-development eLoran system. A recent West Coast USA eLoran test shown here.

[![](http://www.kiwisdr.com/README/eLoran.780px.png)](http://www.kiwisdr.com/README/eLoran.png)

[end-of-document]
