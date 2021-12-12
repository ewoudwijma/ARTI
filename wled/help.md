- [Custom effects](#custom-effects)
- [Quick start](#quick-start)
- [Running examples](#running-examples)
- [Create your own Custom Effects](#create-your-own-custom-effects)
  * [Components](#components)
  * [Functions and variables](#functions-and-variables)
    + [WLED general](#wled-general)
    + [WLED SR](#wled-sr)
    + [Custom Effects](#custom-effects)
    + [Math](#math)
    + [Time](#time)
    + [Pixelblase support](#pixelblase-support)
    + [Serial output](#serial-output)
    + [Details](#details)
  * [Implementation of variables and functions](#implementation-of-variables-and-functions)
- [Current limitations](#current-limitations)
- [trouble shooting](#trouble-shooting)

<small><i><a href='http://ecotrust-canada.github.io/markdown-toc/'>Table of contents generated with markdown-toc</a></i></small>

# Custom effects

Custom effects are effects which are not compiled in the WLED repository but specified by a file (program file) which is interpreted in real time.

The big advantage of this is that effects are not limited by what is made by WLED programmers but anybody can create effects without releasing a new version of WLED. Furthermore any change in the effect code is instantly shown on leds allowing fast developing of effects.

A disadvantage is that the file needs to be loaded, examined and then run in real-time which is 'per definition' slower then pre-compiled code, although performance is promising already and will get better over time.

# Quick start
To get your first Custom Effect running, perform the following steps

* In tab effects, select '⚙️ Custom Effect'

![Custom effect](https://github.com/MoonModules/WLED-Effects/blob/master/Images//CustomEffect.PNG?raw=true)

* In tab Segments, give the segment a name, this will be the name of the Custom Effect

![Segment name](https://github.com/MoonModules/WLED-Effects/blob/master/Images/SegmentName.jpg?raw=true)

* Click on Custom Effect Editor

![Segment name](https://github.com/MoonModules/WLED-Effects/blob/master/Images/CustomEffectsEditor.PNG?raw=true)

* Click on Download wled.json to enable Custom Effects for WLED (needed each time a new version of CE is published)
* Click on Load template to get a 'hello world' example
* Press save and the template will be executed

# Running examples

Custom Effects examples are stored in [Github repository](https://github.com/MoonModules/WLED-Effects/tree/master/CustomEffects/wled)

If you develop effects which you want to share, ask for access on Github.

These effects can be loaded easily within WLED: Give a Custom Effects segment name the same name as an effect in this repository (case sensitive, without .wled), click on Custom Effect Editor and click Download 'effect'.wled and press Save.

Alternatively, if you want all the effects in this folder at once, go to the Custom Effect Editor and click Download presets.json (Segment stop is set to 50! This will overwrite any existing presets you have). Refresh the WLED page or reboot to see the new presets.

![Examples presets](https://github.com/MoonModules/WLED-Effects/blob/master/Images//ExamplesPreset.PNG?raw=true)

# Create your own Custom Effects

A Custom Effects program typically looks like this:

![Example](https://github.com/MoonModules/WLED-Effects/blob/master/Images/Custom%20Effects%20program%20example.PNG?raw=true)

A program contains structures like if statements, for loops, assignments, calls (e.g. renderFrame) etc., commands like setPixelcolor and variables like ledCount.

## Components
* program: Once every effect. Can contain global variables and internal functions. There are 2 special internal functions: renderFrame and renderLed

* Global variables: Once every effect, reused between functions. Variables (global and local) are defined by using an assignment e.g. t=0

* renderFrame: Once every frame

* renderLed: Once every led within a frame

## Functions and variables

Functions and variables give access to the WLED functionality. The list of functions and variables will grow as we go.
A function has parameters (even empty parameters) e.g. setPixelColor(x,y), variables haven't e.g. ledCount.

### WLED general

    "ledCount": {},
    "setPixelColor": {"pixelNr":"int", "color":"int"},
    "leds": {},
    "setPixels": {"leds": "array"},
    "hsv": {"h":"uint8", "s":"uint8", "v":"uint8"},

    "setRange": {"from":"uint16", "to":"uint16", "color":"uint32"},
    "fill": {"color":"uint32"},
    "colorBlend": {"color1":"uint32", "color2":"uint32", "blend":"uint16"},
    "colorWheel": {"pos":"uint8"},
    "colorFromPalette": {"index":"uint8", "brightness":"uint8"},

    "segcolor": {"index":"uint8"},
    "speedSlider": {"return":"uint8"},
    "intensitySlider": {"return":"uint8"},

### WLED SR
    "beatSin": { "bpm":"uint16", "lowest":"uint8", "highest":"uint8", "timebase":"uint32", "phase_offset":"uint8"},
    "fadeToBlackBy": {"fadeBy":"uint8"},
    "iNoise": {"x":"uint32", "y":"uint32"},
    "fadeOut": {"rate":"uint8"},

    "custom1Slider": {"return":"uint8"},
    "custom2Slider": {"return":"uint8"},
    "custom3Slider": {"return":"uint8"},
    "sampleAvg": {"return": "double"},

### Custom Effects
    "counter": {"return": "uint32"},

    "shift": {"delta": "int"},
    "circle2D": {"degrees": "int"}, 

### Math
    "constrain": {"amt":"any", "low":"any", "high":"any"},
    "map": {"x":"int", "in_min":"int", "in_max":"int", "out_min":"int", "out_max":"int"},
    "seed": {"seed": "uint16"},
    "random": {"return": "uint16"},
    "sin": {"degrees": "double", "return": "double"},
    "cos": {"degrees": "double", "return": "double"},
    "abs": {"value": "double", "return": "double"},
    "min": {"value1": "double", "value2": "double", "return": "double"},
    "max": {"value1": "double", "value2": "double", "return": "double"},

### Time
    "hour": {"return":"uint8"},
    "minute": {"return":"uint8"},
    "second": {"return":"uint8"},
    "millis": {"return": "uint32"},

### Pixelblase support
    "time": {"inVal":"double", "return": "double"},
    "triangle": {"t":"double", "return": "double"},
    "wave": {"v":"double", "return": "double"},
    "square": {"v":"double", "t":"double", "return": "double"},

### Serial output
    "printf": {"args": "__VA_ARGS__"}

### Details
* ledcount: number of leds within(!) a segment 
* setpixelColor: currently the second parameter is color from palette!
* leds: one or 2 dimensional array: One index for led strips and 2 indexes for panels. If the leds variable is used an implicit setPixels(leds) will be done each frame! 
* shift: shift all leds left or right (using delta)
* circle2D: puts a dot on a circle using the angle. Used to show a 2D clock, see clock2D.wled
* random: 16 bit random nr
* sin/cos: value between -1 and 1
* hour/minute/second: current time (set in time preferences)
* printf: currently no real printf: prints numbers, max 3

## Implementation of variables and functions

All variables and values are internally stored as doubles and where needed converted to (unsigned) integers, e.g. to WLED functions or operators like %.

Technical details about external variables and functions can be found in arti_wled.h. Look for arti_external_function, arti_set_external_variable and arti_get_external_variable. Some examples:

![Function implementation](https://github.com/MoonModules/WLED-Effects/blob/master/Images/Function%20implementation.PNG?raw=true)

# Current limitations

* Only 1 segment
* no unary operators like - (use 0-1) and ++, --
* no strings


# trouble shooting

* effect crashes: most likely too deeply nested commands (e.g. pixbri = (sin(startVal + millis()/(255- freq)) + 1) * 128), try to split up in more lines.






# old

# How it works

Program files are uploaded to the file system of WLED (/edit) and after selecting effect '⚙️ Custom Effect', the file is opened, it’s content is examined and executed. 


# User interface

Currently, Custom effects are build in [WLED Soundreactive, latest dev version](https://github.com/atuline/WLED/tree/dev). See [here](https://github.com/MoonModules/WLED/wiki/Hardware#software) how to compile or install a precompiled bin.



The following steps are needed to run a Custom Effect:


* Upload files to /edit: The definition file wled.json and default.wled should be uploaded at minimal. 

![Slash Edit](https://github.com/MoonModules/WLED-Effects/blob/master/Images//SlashEditPNG.PNG?raw=true)






# Update from previous version
If you have uploaded files to /edit before and a new version is published, follow this:

mandatory
- upload new wled.json

desirable (to get newest effects)

- remove presets.json and upload new presets.json (change the stop to the nr of leds you have), or copy paste each api command in the file to a preset manually  
- remove all the .wled files and upload the newest wled files.




# How it works in detail

The program file contains commands which adhere to a standard. These commands are specified in a definition file called wled.json.

To run this, a tool called Arduino Real Time Interpreter ([ARTI](https://github.com/ewoudwijma/ARTI)) is used. Actually this tool has been made to make Custom effects possible in WLED but can also be used without WLED. The tool is build using compiler technology and can support not only the WLED definition file but 'any' definition file. 

Arti will run the following steps sequentially: Lexer, Parser, Analyzer, Optimizer and Interpreter. The steps are done once if an affect is selected (SEGENV.call == 0). Interpreter will be executed one time to initialize global variables and functions and then run in a loop executing the function 'renderFrame' and 'renderLed'.

Another definition file can be created if you want to run commands in another coding language. In the Arti Github repository, pas.json is added as a demo to show an example of another definition file. See [below](#Definition-files) how to create another definition file.

# Soundreactive
* Currently this has been added in the WLED Soundreactive / dev branch. As this is not limited to the Soundreactive fork, it could also be added to it's upstream repo: WLED AC. This might be a future step. 

# Preset API command

To create a new custom effect, insert the following API command in a new preset and replace "ColorFade" with the prefix of the filename of the Custom Effect (In this example the filename is ColorFade.wled)

`{"on":true,"bri":128,"transition":7,"mainseg":0,"seg":[{"id":0,"start":0,"stop":30,"grp":1,"spc":0,"of":0,"on":true,"bri":255,"n":"ColorFade","col":[[255,160,0],[0,0,0],[0,0,0]], "fx":187,"sx":128,"ix":128,"f1x":128,"f2x":128,"f3x":128,"pal":0,"sel":true,"rev":false,"rev2D":false,"mi":false,"rot2D":false},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0},{"stop":0}]}`

(fx:187 is id of the Custom Effect)

# Contribute to further development

## Run on Windows
* Clone the ARTI repository
* Run on Windows using CompileAndRunGCC.bat
* Set the right effect in arti_test.cpp e.g. default.wled
* Check created parsetree and log file e.g. default.wled.json and default.wled.log
* Can run on Mac/Linux if CompileAndRunGCC.sh is created

## Deploy on windows
* Use Visual Studio
* Open the repo folder
* Modify code
* Run on windows using CompileAndRunGCC.sh

## Deploy on Arduino
* Use Visual Studio
* Download latest [WLED SR dev repository](https://github.com/atuline/WLED/tree/dev)
* Copy your arti.h / arti_wled_plugin.h to the repository (wled00/src/dependencies/arti). Upload your wled.json or <effect>.wled to /edit
* Build on arduino. See [link](https://github.com/MoonModules/WLED/wiki/Hardware#software).

## Contribute
* Submit a pull request from your clone to the upstream ARTI repository

# Definition files
Definition files define the syntax and semantics of the programming language you want to use. ARTI supports 'any' language as long as it has functions, calls, variables, for, if etc. ... wled.json is an example of this. Any new definition file should contain the following parts:

`{`
  `"meta": {"version": "0.0.4", "start":"program"},`
  `"program": ["PROGRAM","ID","block"],`
  `"block": ...,`

  `"TOKENS":`
  `{`
    `"ID": "ID",`
    `"INTEGER_CONST": "INTEGER_CONST",`
    `"REAL_CONST": "REAL_CONST",`
    `"PROGRAM": "PROGRAM",`
  `},`

  `"SEMANTICS":`
  `{`
    `"program": {"id":"Program", "name":"ID", "block": "block"},`
    `"variable": {"id":"Var", "name":"ID", "type": "type"},`
    `"assign": {"id":"Assign", "name":"varleft", "indices":"indices", "value":"expr"},`
    `"function": {"id":"Function", "name":"ID", "formals": "formals", "block": "block"},`
    `"formal": {"id":"Formal", "name":"ID", "type": "type"},`
    `"call": {"id":"Call", "name":"ID", "actuals": "actuals"},`
    `"for": {"id":"For", "from":"assign", "condition":"expr", "increment": "increment", "block":"block"},`
    `"if": {"id":"If", "condition":"expr", "true":"block", "false": "elseBlock"},`
    `"expr": {"id":"Expr"},`
    `"term": {"id":"Term"},`
    `"varref": {"id":"VarRef", "name":"ID"}`
  `},`

  `"EXTERNALS":`
  `{`
    `"setPixelColor": {"pixelNr":"int", "color":"int"},`
    `"printf": {"args": "__VA_ARGS__"},`
    `"ledCount": {}`
  `}`

`}`

* a meta tag containing version and start, current version of arti.h requires minimal version 0.0.4. Start is the first part of your program. A program is specified by [BNF](https://en.wikipedia.org/wiki/Backus%E2%80%93Naur_form)-like statements in the form of "symbol": "expression". Expression can contain special directives: ? is optional, + is one or more, * is 0 or more.
* SEMANTICS: tells arti how to recognize different parts of the syntax
* EXTERNALS: define predefined functions and variables. They should be defined in arti_<definition>_plugin.h
* For any new definition, arti.h should have an include statement of the plugin file

## References

* https://github.com/rspivak/lsbasi
* https://compilers.iecc.com/crenshaw/
* https://github.com/ewoudwijma/ARTI
