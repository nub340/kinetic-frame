# Banksy Frame

## Description
This is an open-source project for building a kinetic "shredder" frame that, when fully built, mimics the infamous stunt Banksy pulled off at a Sotheby's auction back in 2018. However, where the original device actually shredded the artwork contained within the frame, this one is an illusion. While it really <em>looks</em> like the artwork inside is being shredded, it's actually an artful deception. In reality, a separate pre-shredded copy of the artwork is concealed within the frame, which is cleverly lowered from the bottom of the device while the displayed artwork smoothly scrolls down and out of view, resulting in a very convincing illusion. The device can then be reset to it's original state so the illusion can be enjoyed again and again, indefinitely!

## Authors
- [nub340](https://github.com/nub340)

## Status   
Alpha release. This is the "first light" push which has been fully built and tested by the original author 

See [CHANGELOG](CHANGELOG.md) for more details
  
<!-- ## See what it looks like
Describe what sets this apart from related-projects. Linking to another doc or page is OK if this can't be expressed in a sentence or two.

**Screenshot**: If the software has visual components, place a screenshot after the description; e.g.,

![](https://blah.png) -->

## Tech Stack & Requirements 
- Arduino
- C++ code/sketch
- Custom PCBs via JLCPCB
- 3D printed parts
- Manual fabrication

## Tools & Cosumables Needed
- Hack saw or power saw for cutting aluminum tubing
- Heat gun or blow dryer for heat shrink tubing
- Soldering iron & solder (heavy and thin rosin core)
- Wire strippers/cutters
- Pliers/crimper
- 3D printer
- JB Weld for Metal (or similar)
- Fabric glue

## Materials Required
Description | Qty | Qty Type | Note
--- | --- | --- | ---
[Arduino Nano](https://www.amazon.com/LAFVIN-Board-ATmega328P-Micro-Controller-Arduino/dp/B07G99NNXL) | 1 | pieces | Microcontroller. The brain of the whole device.
[NEMA17 Stepper Motor](https://www.amazon.com/gp/product/B00PNEQ79Q) | 1 | pieces | Low profile recommended for compactness
[A4988 Driver](https://www.amazon.com/HiLetgo-Stepstick-Stepper-Printer-Compatible/dp/B07BND65C8) | 1 | pieces | Drives the stepper motor
[12V-to-5V Buck Converter](https://www.amazon.com/gp/product/B08Q3TKJH5) | 1 | pieces | 5V for Arduino, A4988 driver, and other components
[IR Remote & Receiver](https://www.amazon.com/DWEII-Infrared-Wireless-Control-Raspberry/dp/B09ZTZQFP7) | 1 | pieces | Any will work, may need to update [code](./Code/banksy_shredder_frame.ino#L16)
[Buzzer](https://www.amazon.com/gp/product/B01N7NHSY6) | 1 | pieces | Active or passive will work. Use resistor with passive.
[Resistor](https://www.amazon.com/Resistor-Tolerance-Resistors-Limiting-Certificated/dp/B08QRT11GG) | 1 | pieces | Optional for Active buzzer. Passive use 100ohm.
[4-Pin Connectors](https://www.amazon.com/CQRobot-Connector-Terminal-Industrial-Integrated/dp/B0731NHS9R) | 1 | pieces | Optional. Can solder motor wires directly to PCB
[1" Aluminum Tubing](https://www.mkmetal.net/6061-t6rndtube1x.065x12) | 28 | inches | Top & bottom rollers. 1" x .065" (ID: .87" \| 22.1mm)
[Skateboard Bearings](https://www.amazon.com/gp/product/B07S1B3MS6) | 3 | pieces | JB Weld flush into ends of aluminum tube
[ Heat Shrink Tubing](https://www.amazon.com/gp/product/B0B38T8SC1) | 2 | pieces | (1.25" Diameter x 15" Length) X 2. Shrink over aluminum tubes to make rollers.
[Motor mount spool](./STL/motor-mount-spool.stl) | 1 | pieces | 3D print. Connects stepper motor to the upper roller, and provides a spool for the fishing line.
[Fishing line](https://www.amazon.com/Zebco-Quantum-300210-Outcast-Monofilament/dp/B000KKUO8S) | 3 | feet | 10lb+ recommended to minimize stretching
[Eye hook screws](https://www.amazon.com/gp/product/B0899QTFWS) | 2 | pieces | 10x5mm. Fishing line routing.
[Swivel hook](https://www.amazon.com/Dr-Fish-Interlock-Stainless-Corrosion-Resistanc/dp/B07JQBLZQG) | 1 | pieces | Optional. Small size 14. Attach fishing line to clip
[Shredded Artwork Clip](./STL/clip.stl) | 1 | pieces | Holds the shredded artwork inside frame
[Roller mount pins: long](./stl/roller-mount-pin-long.stl) | 3 | pieces | 3D print. Top & bottom rollers are mounted and rotate on these.
[Roller mount pins: short](./stl/roller-mount-pin.stl) | 4 | pieces | 3D print. Rubber bands attachments to provide tension on bottom roller.
[3/4" Canvas Offset Clips](https://www.amazon.com/gp/product/B01MS39JJP) | 4 | pieces | For mounting picture frame to completed PCB box
[12V Power Adapter](https://www.amazon.com/gp/product/B00Q2E5IXW) | 1 | pieces | Minimum 2A for stepper motor. 8ft cord recommended
[Barrel Connector](https://www.amazon.com/gp/product/B074LK7G86) | 1 | pieces | Connection for 12V power adapter
[Paper Mount Boards](https://www.dickblick.com/items/crescent-mounting-board-15-x-20-x-055-ultra-black)  | 2 | pieces | For internal separator and back pieces. [Cut to size](#mount-boards).
[Card Stock](https://www.amazon.com/gp/product/B0773KWZLG) | 1 | pieces | Optional backing. Glue to back of artwork copy before shredding for better support and durability.
[12" x 16" Picture Frame](https://www.etsy.com/listing/630386658/west-frames-elegance-french-ornate) | 1 | pieces | Minimum 1-1/2" Moulding to hide motor & electronics
[Custom PCB: Left & Right](./PCB/Gerber_Left_Right.zip) | 1 | pieces | Order from [JLCPCB](https://jlcpcb.com/)
[Custom PCB: Top, Bottom, & Supports](./PCB/Gerber_Top_Bottom.zip) | 1 | pieces | Order from [JLCPCB](https://jlcpcb.com/)
[Main Artwork Canvas: 13.5" x 36"](https://www.bagsoflove.com/unframed-canvas-photo-prints) | 1 | pieces | The main un-shredded artwork that will be displayed in the frame.
[Shredded Artwork Canvas: 13.5" x 12 - 16"](https://www.bagsoflove.com/unframed-canvas-photo-prints) | 1 | pieces | The shredded artwork that will be lowered from the bottom of the frame. Doesn't have to be full 16" as only around half is ever exposed.


## Getting the Printed Circuit Boards (PCBs) Made
You can have the PCBs made via JLCPCB. They require a minimum of 5 copies of any PCB. Follow the steps below to order:

- Browse to [JLCPCB](https://jlcpcb.com/)
- Click "Instant Quote" button
- Upload [Gerber file for Left & Right](./PCB/Gerber_Left_Right.zip) pieces
- Under the <b>PCB Specifications</b> section, select the following:
    - <b>Different Design</b>: ```2```
    - <b>Delivery Format</b>: ```Panel by Customer```
    - <b>PCB Thickness</b>: ```2.0```
    - <b>PCB Color</b>: <em>Select your desired color</em>
- Click "Save to Cart".
- Repeat for [Top, Bottom, and Supports](./PCB/Gerber_Top_Bottom.zip) Gerber file
    - Only difference is choose <b>Different Design</b>: ```3``` this time
- You will end up with 2 items in your cart. Proceed to checkout when you are ready to place your order.
- You can choose ```1.6``` PCB thickness to save money, but ```2.0``` is way sturdier and recommended.
- <b>NOTE</b>: ```Gerber files are just zip files. Upload the whole zip, no need to extract first.```


## Fabricating the Rollers
You'll need to fabricate the upper & lower rollers from a few components:
- Aluminum tubing
- Skateboard bearings
- Heat shrink tubing
- JB weld for metal/aluminum (marine works well)

Follow these steps:
- Cut the aluminum tubes to length:
    - Upper: ```13-3/4"``` (extra space needed for 3D printed motor mount spool)
    - Lower: ```14"```
- Install bearings:
    - You'll need three bearings total. One for the upper, and two for the lower.
    - Test fit the bearings into the ends of the tubes. Make sure they will fit nice and flush with the ends of the tubes. You may need to sand the inside of the tubes slightly if it's too tight a fit.
    - Remove all three bearings.
    - Install one bearing at a time with the following procedure:
        - Apply/scrape a <em>little bit</em> of JB weld all around the inside perimeter of the tube openings such that when the bearing is pressed in, it will make contact with both the outer bearing race and inner tube wall. 
        - Press bearing into tube end so that it sits nice and flush with the tube end.
        - Don't use too much JB Weld as you don't want it to get onto the inner bearing race.
        - Let the JB Weld harden/cure thoroughly.
        - Ensure the bearing spins freely. See [notes](#bearing-notes) below if not.
- Apply heat shrink:
    - Cut two ~15" lengths of heat shrink tubing and shrink them onto each roller, both upper and lower.
    - Trim the excess heat shrink tubing with a razor blade so that it's perfectly flush with the ends of the tubes and bearings, with no overhang.
- Install 3D printed motor mount spool
    - Press into the tube opening opposite of the bearing of the upper roller tube
    - Ensure that it is fully pressed in and seated so that it grips the tube well.
    - Total length of tube plus motor mount should be 14" 

NOTES:
<a name="bearing-notes"></a>
- If you get JB Weld on the center bearing race and it doesn't want to spin, you can try to use your finger, or stick a large pen or something similar through the hole until it grabs onto the sides of the hole, and then spin the tube to try and break bond and get it spinning freely.
- You might be able to use PVC tubing instead of aluminum, but the PVC might warp when applying the heat shrink tubing.
- If you got the right tubing and skateboard bearings, they should fit right into the ends of the tubes. The outer diameter of the bearings are 22mm, so the inside diameter of the tubes need to be just over 22mm.


## Fabricating the Artwork
The shredder frame requires two separate copies of the same artwork to function. One for the main un-shredded art that will be visible in the frame when in the default/normal state, and one for the shredded piece that is lowered from the frame.

The two artworks can be fabricated in many ways so feel free to experiment with different approaches. If you find one that works well that is not covered here, please write up an addendum ```.md``` and make a pull request for it to be included in the main repository.

I have tried a few different approaches myself and found having the canvas made via a service like [bagsoflove](https://www.bagsoflove.com/unframed-canvas-photo-prints) provides the best results. The artwork is printed directly onto a high quality canvas which looks great, and avoids any issues with the picture starting to come off after repeated times going around the rollers.

Gluing a separate artwork onto a blank piece of canvas or vinyl is another (cheaper) approach, but presents potential peeling issues as it must be able to rotate around the lower roller over and over again. If it starts to peel off, it will inevitably start causing jams. I had decent results using a piece of vinyl instead of canvas and using a high strength 3M spray adhesive.

You can also simply make your own stencils and spray paint directly on a blank canvas, although this might be harder to make two identical copies.


### Main Un-shredded Artwork
- 13.5" x 36" made into a belt/loop
- Mounted on rollers
- Have made
    - https://www.bagsoflove.com/
    - https://www.contrado.com/
    - You'll need to upload a high quality image (i.e. 200 DPI) in the right dimensions.
    - [Example image](./Images/banksy_13.5x52_fiberboard_200DPI.png)
        - Single image contains both the un-shredded (upper) and shredded (lower) artworks in one. 
        - It will be delivered as one big canvas, so you'll need to separate the upper and lower canvases by cutting with scissors or fabric cutter, etc. Cut very precisely along the horizontal dotted line separating the upper and lower portions.
        
        - cut the bottom image out and glue it to cardstock before manually shredding for best results and durability. 
        - Only shred bottom half of shredded artwork canvas.
        - Trim the upper un-shredded artwork canvas so that it's 13-1/2" wide, and ~37" tall. Use high quality fabric glue to make into a belt/loop. You only need about an inch of overlap for glue to make a good bond. 
        - The fiberboard-like background on the upper canvas should loop around and perfectly match with the top to make a perfect 36" long belt. 
- Make yourself
    - Print, Stencil, or Glue artwork onto a 13.5" x 36" canvas about 3/4 of the way down. 
    - Make into a "belt" by gluing the ends together. 
    - Seam should be near the bottom in back when artwork is centered in frame on the rollers. Far enough down so that it never reaches the upper roller when in the fully shredded state.


## Fabricating the Shredded Artwork
Duplicate artwork above. Glue canvas to cardstock for best results, or just use the cardstock. Shred bottom 50% or so. By hand or with a paper shredder you can stop & reverse.


## 3D Printed Parts
- [Motor mount spool](./STL/motor-mount-spool.stl) - Connects the motor to the upper roller. Also provides a spool for the fishing line and acts as a winch for lowering and raising the shredded artwork hidden inside.
- [Roller mount pin: long](./STL/roller-mount-pin-long.stl) - Provides a spindle for the upper & lower rollers to mount and rotate on.
- [Roller mount pin: short](./STL/roller-mount-pin.stl) - Provides an attachment point for the tensioning rubber bands for the lower roller.
- [Artwork clip](./STL/clip.stl) - Provides an attachment point for the shredded artwork to hang from. 

It's recommended to print these components with PETG or PLA @ .15mm or better resolution and 15% infill. Be sure to use the appropriate slicer for your printer. 

## Mount Boards
<a name="mount-boards"></a>
- Separator: 14.25" x 18.25" 
- Backing: 14.5" x 18.75"
    - bend the bottom slightly so that the shredded artwork is pushed away from the wall a little

## Dependencies

Describe any dependencies that must be installed for this software to work.
This includes programming languages, databases or other storage mechanisms, build tools, frameworks, and so forth.
If specific versions of other software are required, or known not to work, call that out.

## Installation

Detailed instructions on how to install, configure, and get the project running.
This should be frequently tested to ensure reliability. Alternatively, link to
a separate [INSTALL](INSTALL.md) document.

## Configuration

If the software is configurable, describe it in detail, either here or in other documentation to which you link.

## Usage

Show users how to use the software.
Be specific.
Use appropriate formatting when showing code snippets.

## How to test the software

If the software includes automated tests, detail how to run those tests.

## Known issues

Document any known significant shortcomings with the software.

## Getting help

Instruct users how to get help with this software; this might include links to an issue tracker, wiki, mailing list, etc.

**Example**

If you have questions, concerns, bug reports, etc, please file an issue in this repository's Issue Tracker.

## Getting involved

This section should detail why people should get involved and describe key areas you are
currently focusing on; e.g., trying to get feedback on features, fixing certain bugs, building
important pieces, etc.

General instructions on _how_ to contribute should be stated with a link to [CONTRIBUTING](CONTRIBUTING.md).


----

## Open source licensing info
1. [TERMS](TERMS.md)
2. [LICENSE](LICENSE)
3. [CFPB Source Code Policy](https://github.com/cfpb/source-code-policy/)


----

## Credits and references

1. Projects that inspired you
2. Related projects
3. Books, papers, talks, or other sources that have meaningful impact or influence on this project# kinetic-frame
# kinetic-frame
# kinetic-frame
