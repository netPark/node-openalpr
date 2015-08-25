var nativeBio = {}
    fs = require ("fs")
    child_process = require ("child_process")
    os = require ("os")
    path = require ("path"),
    native_lib_path = path.join (__dirname, '../build/Release/node_openalpr.node');

if(fs.existsSync (native_lib_path)) {
    nativeLPR = require (native_lib_path);
}
else {
    nativeLPR = require (path.join (__dirname, "../release/" + process.platform + "_" + process.arch + "/node_openalpr.node"));
}

/**
 * OpenALPR NodeJS wrapper class
 */
function OpenALPR () {
	var object = this;
	var initialized = false;
	var started = false;
	var callback;
	var loop;
	var in_loop = false;
	
	/**
	 * Start the OpenALPR process
	 * @param int count Amount of OpenALPR threads to startup [Default: System CPU count]
	 * @param bool start_queue Auto-start queuing [Default: true]
	 */
	this.Start = function (config, runtime, count, start_queue) {
		if (initialized) {
			return true;
		}
		
		if (!config && os.platform () === "win32") {
			config = path.join (__dirname, "../openalpr.conf");
		}
		
		if (!runtime && os.platform () === "win32") {
			runtime = path.join (__dirname, "../openalpr_runtime/");
		}
		
		var cpu_count = os.cpus ().length;
		
		initialized = nativeLPR.Start (config || "", runtime || "", count || cpu_count, function (reason) {
			console.log ("OpenALPR Stopped " + reason);
		});
		
		if (start_queue !== false) {
			this.StartQueue ();
		}

		return initialized;
	}
	
	/**
	 * Stops OpenALPR and kills off all queued up pictures
	 */
	this.Stop = function () {
		this.StopQueue ();
		return nativeLPR.Stop ();
	}
	
	/**
	 * Starts the queue to update 30 times a second
	 */
	this.StartQueue = function () {
		if (!initialized || typeof loop === "object") {
			return;
		}
	
		loop = setInterval (this.queueLoop, 32);
	}
	
	/**
	 * Stops the queue from auto-updating
	 */
	this.StopQueue = function () {
		if (typeof loop !== "object") {
			return;
		}
	
		clearInterval (loop);
		loop = null;
	}
	
	/**
	 * Get the version of OpenALPR detected
	 * @return string Version (x.x.x)
	 */
	this.GetVersion = function () {
		if (!initialized) {
			throw "OpenALPR Not Initialized";
		}
		
		return nativeLPR.GetVersion ();
	}
	
	/**
	 * Begin the process of identifying a license plate
	 * @param string path Full path to image
	 * @param object Options Options to apply to this identify (state "oh", prewarp, detectregion, etc)
	 * @param function callback Callback function on function (error, output)
	 * @return string Status - "queued" or "working"
	 */
	this.IdentifyLicense = function (path, options, callback) {
		if (!initialized) {
			throw "OpenALPR Not Initialized";
		}
	
		if (typeof callback !== "function" && typeof options !== "function") {
			throw "Callback method required";
		}
		else if (typeof options === "function") {
			callback = options;
		}
		
		if (!fs.existsSync (path)) {
			throw "File does exist";
		}
		
		if (typeof options === "undefined" || typeof options === "function" || !options) {
			options = {};
		}
		
		var regions = [];
		if (options.regions && options.regions.length) {
			for (var r in options.regions) {
				var region = options.regions[r];
				if (!region.x || !region.y || !region.width || !region.height) {
					continue;
				}
				
				regions.push ([region.x, region.y, region.width, region.height]);
			}
		}
		
		return nativeLPR.IdentifyLicense (path, options.state || "", options.prewarp || "", options.detectRegion || false, regions, function (error, output) {
			callback (error, JSON.parse (output));
		});
	}
	
	/**
	 * Checks the queue to see if any instances are free to begin work
	 */
	this.queueLoop = function () {
		if (in_loop) {
			return;
		}
	
		in_loop = true;
		nativeLPR.CheckQueue ();
		in_loop = false;
	}
}

module.exports = new OpenALPR ();
