function Actuator(name, type, gpio, value, min, max) {
    var self = this;
    self.name = ko.observable(name);
    self.type = ko.observable(type);
    self.gpio = ko.observable(gpio);
    self.value = ko.observable(value);
    self.min = ko.observable(min);
    self.max = ko.observable(max);

    self.enabled = ko.computed(function () {
        return self.value() ? "btn btn-success" : "btn btn-secondary";
    });

    self.isButton = ko.computed(function () {
        return self.type() == 0;
    });

    self.value.subscribe(function (value) {
        var name = self.name();
        var val = value;
        if (typeof value === "boolean") {
            val = value ? 1 : 0;
        }

        console.log("New value: ", value, JSON.stringify({ "name": self.name(), "value": val }));
        $.post("/api/actuator/change",
            { "name": self.name(), "value": val },
            function () {
                console.log("sent");
            });

    });

    self.toggle = function () {
        self.value(!self.value());
    }
}

function Settings(general, network, timers, sensors, actuators, thresholds) {
    var self = this;
    self.general = ko.observable(general);
    self.network = ko.observable(network);
    self.timers = ko.observableArray(timers);
    self.sensors = ko.observableArray(sensors);
    self.actuators = ko.observableArray(actuators);
    self.thresholds = ko.observableArray(thresholds);
}

function SettingsGeneral(dst){
    var self = this;
    self.dst = ko.observable(dst);
}

function SettingsNetwork(hostname){
    var self = this;
    self.hostname = ko.observable(hostname);
}

function SettingsTimer(name, actuator, on, off, inverted){
    var self = this;
    self.name = ko.observable(name);
    self.actuator = ko.observable(actuator);
    self.on = ko.observable(on);
    self.off = ko.observable(off);
    self.inverted = ko.observable(inverted);
}

function SettingsSensor(name, type, gpio, address, enabled){
    var self = this;
    self.name = ko.observable(name);
    self.type = ko.observable(type);
    self.gpio = ko.observable(gpio);
    self.address = ko.observable(address);
    self.enabled = ko.observable(enabled);
}

function SettingsActuator(name, gpio, type, min, max){
    var self = this;
    self.name = ko.observable(name);
    self.type = ko.observable(type);
    self.gpio = ko.observable(gpio);
    self.min = ko.observable(min);
    self.max = ko.observable(max);
}

function SettingsThreshold(name, duration, threshold, comparator, actuator, inverted, gap, sensor, sensor_type){
    var self = this;
    self.name = ko.observable(name);
    self.duration = ko.observable(duration);
    self.threshold = ko.observable(threshold);
    self.comparator = ko.observable(comparator);
    self.actuator = ko.observable(actuator);
    self.inverted = ko.observable(inverted);
    self.gap = ko.observable(gap);
    self.sensor = ko.observable(sensor);
    self.sensor_type = ko.observable(sensor_type);
}



// Overall viewmodel for this screen, along with initial state
function TerraESPViewModel() {
    var self = this;

    // Actuators
    self.actuators = ko.observableArray([]);
    $.getJSON("/api/actuators", function (data) {
        console.log("Actuator", data);
        self.actuators($.map(data, function (a) {
            return new Actuator(a.name, a.type, a.gpio, a.value, a.min, a.max);
        }));
    });

    // chart
    self.chartData = ko.observable({
        labels: ko.observableArray([]),
        datasets: ko.observableArray([])
    });
    self.chartOptions = {
        observeChanges: true,
        throttle: 100,
        scales: {
            xAxes: [{
                type: 'time',
                time: {
                    unit: 'minute',
                    stepSize: 30
                }
            }],
            yAxes: [{
                ticks: {
                    suggestedMin: 0,
                    suggestedMax: 100
                }
            }]
        }
    };

    $.getJSON("/api/sensors/all", function (data) {
        console.log("Sensors", data);

        // add time labels
        var labels = data.time.map(function (x) { return new Date(x * 1000); });
        var datasets = [];

        // add sensor data
        data.sensors.forEach(e => {
            if ('temperature' in e && e.temperature.length > 0) {
                datasets.push({
                    label: "Temperature: " + e.name,
                    backgroundColor: "rgba(220,0,0,0.2)",
                    borderColor: "rgba(220,0,0,1)",
                    pointColor: "rgba(220,0,0,1)",
                    pointStrokeColor: "#fff",
                    pointHighlightFill: "#fff",
                    pointHighlightStroke: "rgba(220,0,0,1)",
                    data: e.temperature
                });
            }
            if ('humidity' in e && e.humidity.length > 0) {
                datasets.push({
                    label: "Humidity: " + e.name,
                    backgroundColor: "rgba(0,0,220,0.2)",
                    borderColor: "rgba(0,0, 220,1)",
                    pointColor: "rgba(0,0,220,1)",
                    pointStrokeColor: "#fff",
                    pointHighlightFill: "#fff",
                    pointHighlightStroke: "rgba(0,0,220,1)",
                    data: e.humidity
                });
            }
        });

        // add events
        var events = [];
        data.events.forEach(e => {
            events.push({
                x: new Date(e.time * 1000),
                y: 10,
                r: 5
            });
        });
        datasets.push({
            label: "events",
            backgroundColor: "rgba(0,220,0,0.2)",
            borderColor: "rgba(0,220,0,1)",
            pointColor: "rgba(0,220,0,1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "rgba(0,255,0,1)",
            data: events,
            type: "bubble"
        });

        // add thresholds
        data.thresholds.forEach(e => {
            datasets.push({
                label: "Threshold: " + e.name,
                backgroundColor: "rgba(150,150,150,0.2)",
                borderColor: "rgba(150,150,150,1)",
                pointColor: "rgba(150,150,150,1)",
                pointStrokeColor: "#fff",
                pointHighlightFill: "#fff",
                pointHighlightStroke: "rgba(100,100,100,1)",
                data: [
                    { x: labels[0], y: e.threshold}, 
                    { x: labels[labels.length-1], y: e.threshold}, 
                ]
            });
        });

        // update chart
        self.chartData({
            labels: labels,
            datasets: datasets
        });
    });

    // Settings
    self.settings = ko.observable();
    $.getJSON("/api/config", function (data) {
        console.log("config", data);
        var general = SettingsGeneral(data.general.dst);
        var network = SettingsNetwork(data.network.hostname);
        var timers = Array();
        var sensors = Array();
        var actuators = Array();
        var thresholds = Array();
        
        // add Actuators
        data.actuators.forEach(e => {
            actuators.push(new SettingsActuator(e.name, e.gpio, e.type, e.min, e.max));
        });

        // add timers
        data.timers.forEach(e => {
            timers.push(new SettingsTimer(e.name, actuators.find(a => a.name() == e.name), e.on, e.off, e.inverted));
        });

        // add sensors
        data.timers.forEach(e => {
            sensors.push(new SettingsSensor(e.name, e.type, e.gpio, e.address, e.enabled));
        });


        // add thresholds
        data.timers.forEach(e => {
            thresholds.push(new SettingsThreshold(e.name, e.duration, e.threshold, e.comparator, 
                actuators.find(a => a.name() == e.name), e.inverted, e.gap,
                sensors.find(a => a.name() == e.name), e.sensor_type));
        });

        var settings = new Settings(general, network, timers, sensors, actuators, thresholds);
        console.log("Settings: ", ko.toJSON(settings));
        self.settings(settings);
    });
}

var model = new TerraESPViewModel();
ko.applyBindings(model);