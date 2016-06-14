
var device = null;
var brightness = 0;

function logDeviceState() {
    console.log("  device.name          : " + device.name);
    console.log("  device.props         : " + device.props);
}


module.exports = {
    initDevice: function (dev) {
        console.log("Javascript initialized.");
        device = dev;
        logDeviceState();
    },

    turnOn: function () {
        console.log("turnOn called.");
        brightness = 100;
        logDeviceState();
    },

    turnOff: function () {
        console.log("turnOff called.");
        brightness = 0;
        logDeviceState();
    }

}

Object.defineProperty(module.exports, "brightness", {
    get: function () {
        console.log("getting brightness property: " + brightness);
        logDeviceState();
        return brightness;
    },
    set: function (value) {
        console.log("setting brightness property to: " + value);
        brightness = value;
        logDeviceState();
    }
});

