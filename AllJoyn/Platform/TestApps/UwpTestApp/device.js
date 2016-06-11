function logDeviceState(device) {
    console.log("  device.name          : " + device.name);
    console.log("  device.props         : " + device.props);
}


module.exports = {
    device: null,
    initDevice: function (dev) {
        console.log("initDevice called...");
        return new Promise((resolve, reject) => {
            setTimeout(() => {
                this.device = dev;
                logDeviceState(this.device);
                console.log("initDevice completed.");
                resolve();
            }, 1000);
        });
    },

    turnOn: function () {
        console.log("turnOn called...");
        return new Promise((resolve, reject) => {
            setTimeout(() => {
                logDeviceState(this.device);
                console.log("turnOn completed.");
                resolve();
            }, 1000);
        });
    },

    turnOff: function () {
        console.log("turnOff called.");
        return new Promise((resolve, reject) => {
            setTimeout(() => {
                logDeviceState(this.device);
                console.log("turnOff completed.");
                resolve();
            }, 1000);
        });
    },

    setBrightness: function (brightness) {
        console.log("setBrightness(" + brightness + ") called...");
        return new Promise((resolve, reject) => {
            setTimeout(() => {
                logDeviceState(this.device);
                console.log("setBrightness completed.");
                resolve();
            }, 1000);
        });
    }

}

// globals for JxCore host
global.initDevice = module.exports.initDevice;
global.turnOn = module.exports.turnOn;
global.turnOff = module.exports.turnOff;
global.setBrightness = module.exports.setBrightness;
