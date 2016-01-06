node-openalpr
============

This package binds [OpenALPR](https://github.com/openalpr/openalpr) with Node.js

Version: 1.1.0 - Released January 4th, 2016

```
Changelog:

1.1.1	- Fix for building on OSX
1.1.0   - Updated OpenALPR to version 2.2, should fix Linux and OSX builds
1.0.9	- Updated OpenALPR build and US region data
1.0.7	- Added the capability to specify regions
1.0.6	- Slowed down the event loop to 30 times per second
1.0.1:5	- Documentation changes
1.0.0	- Initial release
```

# Installation and Example

Use npm to get the node-openalpr package. We'll attempt to use node-pre-gyp to compile from source, but if
that's not possible we'll fallback to precompiled binaries.

### Linux

0. Install [OpenALPR](https://github.com/openalpr/openalpr/wiki/Compilation-instructions-(Ubuntu-Linux))
0. Run ```npm install node-openalpr```

### Windows
0. Run ```npm install node-openalpr```

### OS X
0. Install OpenALPR
	- [OpenALPR](https://github.com/openalpr/openalpr/wiki/Compilation-instructions-(OS-X))
	- using Homebrew(openalpr v2.2.0)
0. Run ```npm install node-openalpr```


### Example

```javascript
var openalpr = require ("node-openalpr");

function identify (id, path) {
	console.log (openalpr.IdentifyLicense (path, function (error, output) {
		var results = output.results;
        console.log (id +" "+ output.processing_time_ms +" "+ ((results.length > 0) ? results[0].plate : "No results"));
	
		if (id == 349) {
			console.log (openalpr.Stop ());
		}
	}));
}

openalpr.Start ();
openalpr.GetVersion ();

for (var i = 0; i < 350; i++) {
	identify (i, "lp.jpg");
}
```

### Methods

This is a breakdown of all of the methods available for node-openalpr. Start needs to be called before any other method.

* `openalpr.Start ([config[, runtime[, count[, start_queue]]]])` - Initializes OpenALPR with default settings
  * config - Path to configuration file. On Windows defaults to the config file in node-openalpr directory, on Linux defaults to openalpr installation
  * runtime - Path to runtime data. On Windows defaults to "openalpr_runtime" folder in node-openalpr directory, on Linux defaults to openalpr installation
  * count - Number of concurrent OpenALPR processes to run - defaults to CPU core count
  * start_queue - Auto start queue monitoring thread - defaults to true
* `openalpr.Stop ()` - Stops the OpenALPR processes and clears out any queued images
* `openalpr.StartQueue ()` - Starts the OpenALPR queue monitoring thread (normally started automatically after calling Start ())
* `openalpr.StopQueue ()` - Stops the OpenALPR queue monitoring thread
* `openalpr.queueLoop ()` - Method used in checking queue - can be called manually if start_queue is false for finer control
* `openalpr.IdentifyLicense (path, options/callback[, callback])` - Begins the process of identifying a license from the given image, returns "working" or "queued" status result
  * path - Path to image - if image does not exist an exception will be thrown
  * callback/options - Additional options for the image or a callback
    * options.state         (string)  - State ("oh") license plates are in for additional validation
    * options.prewarp       (string)  - Prewarp configuration information
    * options.detectRegion  (boolean) - Use detect region functionality of OpenALPR? (slower)
    * options.regions       (array)   - Specify the regions of the image to work on (format: [{ x: 0, y: 0, width: 0, height: 0 }, ...]
  * callback - Callback with results: function (errors, output)
* `openalpr.GetVersion ()` - Get the version of OpenALPR currently being run against

# How to Compile

0. [Download and install io.js v3.0.0+](https://iojs.org/en/index.html)
0. [Download and install git](https://git-scm.com/downloads)
0. [Download and install cmake](https://cmake.org/download/)

#### Windows

0. [Download and install Visual Studio 2013/2015](https://www.visualstudio.com/)
0. Run PowerShell ISE as an administrator and execute: Set-ExecutionPolicy RemoteSigned
0. Run openalpr-install.ps1
0. Take output from openalpr/windows/build/dist and put into "lib" and "release/win32" folder in node-openalpr
0. Run npm install

#### Linux

0. Run openalpr-install.sh
0. Run npm install

# Features, Bugs and Collaborating

All of the code is provided as-is. We will not provide on-going support for any bugs that may be found. Please submit bug
and features requests - we will review them but we do not garunteed that they will be addressed. Pull requests are welcome 
and we'll review them as quickly as we can.
