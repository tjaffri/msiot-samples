function logDeviceState(device) {
    console.log("  device.name          : " + device.name);
    console.log("  device.props         : " + device.props);
}


module.exports = {
    device: null,
    initDevice: function (dev) {
        console.log("Javascript initialized.");
        this.device = dev;
        logDeviceState(this.device);
    },

    turnOn: function () {
        console.log("turnOn called.");
        logDeviceState(this.device);
    },

    turnOff: function () {
        console.log("turnOff called.");
        logDeviceState(this.device);
    },

    setBrightness: function (brightness) {
        console.log("setBrightness called with value: " + brightness);
        logDeviceState(this.device);
    }

}

// globals for JxCore host
global.initDevice = module.exports.initDevice;
global.turnOn = module.exports.turnOn;
global.turnOff = module.exports.turnOff;
global.setBrightness = module.exports.setBrightness;
