[updated 22 April 2026]

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
* [Online Store](https://kiwisdr.nz)
* [Project webpage](http://www.kiwisdr.com)
* [Operating information: installation, operation, FAQ](http://www.kiwisdr.com/info)
* [User Forum](http://forum.kiwisdr.com)
* Latest [source code commits](https://github.com/jks-prv/KiwiSDR/commits/master)
* Previous [source code commits](https://github.com/jks-prv/Beagle_SDR_GPS/commits/master)
* [Design review document](http://kiwisdr.com/docs/KiwiSDR/KiwiSDR.design.review.pdf)
* [KiwiSDR 1 Schematic](http://www.kiwisdr.com/docs/KiwiSDR/kiwi.schematic.pdf)
* [KiwiSDR 2 Schematic](http://www.kiwisdr.com/docs/KiwiSDR/kiwi-2.schematic.pdf)

### Description
This SDR is a bit different. It has a web interface that can be used by up to four separate listeners. Each one listening and tuning an independent frequency simultaneously. See the screenshots below.

### Components
* SDR covering the 10 kHz to 30 MHz (VLF-HF) spectrum.
* Web interface based on [OpenWebRX](https://github.com/ha7ilm/openwebrx) from András Retzler, HA7ILM.
* Integrated software-defined GPS receiver from Andrew Holme's [Homemade GPS Receiver](http://www.aholme.co.uk/GPS/Main.htm).
* LTC/Analog Devices 14-bit 65 MHz ADC.
* Xilinx/AMD Artix-7 A35 FPGA.
* Maxim/Analog Devices MAX2769B GPS front-end.
* Kiwi board works with BeagleBone Green, Black, BBAI or BBAI-64.

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

Give the live receivers a try at the links above. You'll need a recent version of a modern web browser that supports HTML5. The web interface works, with some problems, on mobile devices. But there is no mobile version of the interface yet.

A second generation device, the [KiwiSDR 2](https://forum.kiwisdr.com/index.php?p=/discussion/2986/kiwisdr-2-prototypes-working/p1), is now in production.

### Objectives

We wanted to design an SDR that provides certain features, at a low price point, that we felt weren't covered by current devices. The SDR must be web-accessible and simple to setup and use.
We also wanted to provide a self-contained platform for experimentation with SDR and GPS techniques. The TDoA extension is an example.

Most importantly, we wanted to see a significant number of web-enabled, wide-band SDRs deployed in diverse locations world-wide because that makes possible some really interesting applications and experiments. Over 700 Kiwis are publicly available currently.

### Operation

Users can purchase just the KiwiSDR board or a complete unit consisting of the board, BeagleBone Green (software pre-installed), enclosure, and GPS antenna (see [here](http://www.kiwisdr.com/)).
The software will try to automatically open up an incoming port through whatever Internet firewall/router may exist on the local network, but the user may have to perform this step manually. A reverse proxy service is available.

An antenna solution must be provided. An adequate power supply (e.g. 5V @ 2A) will also be required.

Four channels of audio and waterfall streamed over the Internet 24/7 requires about 30 GB per month. This is a common cap for many residential broadband plans. An automatic dynamic-DNS system is already part of the software so a web link to the SDR is immediately available with no configuration. Of course the system can be configured to only allow connections from the local network and ignore Internet connection requests.

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

What about the application of the Solomonoff induction
to the RF sensing of human brain?
https://en.wikipedia.org/wiki/Talk:Solomonoff%27s_theory_of_inductive_inference

If we assume that the algorithmic complexity
of neural processes is relatively small
(you could take a look at the Potapov monography
for some arguments)

As far as I know a lot of neural processes in human brain are electric (or electro-chemistry)
in nature. They have a little power and a small
frequency (~ 1-1000 Hz).
So they emit very low frequency radio waves.

You can detect those radio waves on
small antennas if the impedance of such antennas
is matched. This is basically the citation
from the book on electrodynamics.
I uploaded one such book on Twirpix site.

So you can create a lot of such receivers

microstrip filter to filter very high frequencies
impedance matched microstrip antenna
resistor for noise for oversampling
very fast comparator to sample signal
in a very large array on a chip using
standard CMOS or some sort of full-custom process
(maybe even with some new materials)

BeamForming and large arrays of digital correlators with sub-mm positioning accuracy
could be achived.

so it seems there is no theoretical
obstacles to implement some sort of
RF human brain sensing or even control
if you can implement reverse structure
with array of a large amount of RF amplifiers
with sub-mm beam forming accuracy

BTW I wrote that remark:
https://en.wikipedia.org/wiki/Talk:Solomonoff%27s_theory_of_inductive_inference

What about the application of the Solomonoff induction to the person to person information exchange?

It seems the minimum description length principle could be used to deduce minimum ethics. At least you can compare ethics by the length. What about the following question: how to shoot met-art girl in the dark underground? ~2026-24709-41 (talk) 16:43, 8 June 2026 (UTC)

at least we can ask a question: does russian met-art model from a pic want some money?
and what obstacles do that guess so hard?
why it is so hard to contact her directly?
she lose money, interesting person lose opportunity to make some photos.

also, it seems the economy without coordination primitives based on physics or mathematics
like gold or maybe Bitcoin or maybe some sort of other physical objects or processes
is just a chaos.
Economic calculation based on fiat money is impossible just from scientific methodology
https://en.wikipedia.org/wiki/Talk:Solomonoff%27s_theory_of_inductive_inference

It seems it is proof of the famous Mises calculation argument
https://mises.org/online-book/economic-calculation-socialist-commonwealth

https://transitional-writes.dreamwidth.org/44972.html
Kirill Abramovich
Samara, Russia

and the most important question:
what about the minimal ethics deduced from the MDL principle?
is it exist?

what was the obstacle between russian met-art girls and engineers?
