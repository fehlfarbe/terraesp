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
    self.general = ko.observable(general === undefined ? new SettingsGeneral() : general);
    self.network = ko.observable(network);
    self.timers = ko.observableArray(timers);
    self.sensors = ko.observableArray(sensors);
    self.actuators = ko.observableArray(actuators);
    self.thresholds = ko.observableArray(thresholds);

    self.flatten = function () {
        var data = {};
        data["general"] = self.general();
        data["network"] = self.network();

        console.log(data);
        console.log(self.general);

        data["timers"] = new Array();
        self.timers().forEach(e => {
            var t = Object.assign({}, e);
            t.actuator = e.actuator().name();
            data["timers"].push(t);
        });

        data["sensors"] = self.sensors();
        data["actuators"] = self.actuators();

        data["thresholds"] = new Array();
        self.thresholds().forEach(e => {
            var t = Object.assign({}, e);
            console.log(e);
            t.actuator = e.actuator().name();
            t.sensor = e.sensor().name();
            data["thresholds"].push(t);
        });


        return data;
    }
}

function SettingsGeneral(gmt_offset, dst) {
    var self = this;
    self.gmt_offset = ko.observable(gmt_offset);
    self.dst = ko.observable(dst);
}

function SettingsNetwork(hostname) {
    var self = this;
    self.hostname = ko.observable(hostname);
}

function SettingsTimer(name, actuator, on, off, inverted) {
    var self = this;
    self.name = ko.observable(name);
    self.actuator = ko.observable(actuator);
    self.on = ko.observable(on);
    self.off = ko.observable(off);
    self.inverted = ko.observable(inverted);
}

function SettingsSensor(name, type, gpio, address, enabled) {
    var self = this;
    self.name = ko.observable(name);
    self.type = ko.observable(type);
    self.gpio = ko.observable(gpio);
    self.address = ko.observable(address);
    self.enabled = ko.observable(enabled);
}

function SettingsActuator(name, gpio, type, min, max) {
    var self = this;
    self.name = ko.observable(name);
    self.type = ko.observable(type);
    self.gpio = ko.observable(gpio);
    self.min = ko.observable(min);
    self.max = ko.observable(max);
}

function SettingsThreshold(name, duration, threshold, comparator, actuator, inverted, gap, sensor, sensor_type) {
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
    // self.chartData = ko.observable({
    //     labels: ko.observableArray([]),
    //     datasets: ko.observableArray([])
    // });
    // self.chartOptions = {
    //     observeChanges: true,
    //     throttle: 100,
    //     scales: {
    //         xAxes: [{
    //             type: 'time',
    //             time: {
    //                 unit: 'minute',
    //                 stepSize: 30
    //             }
    //         }],
    //         yAxes: [{
    //             ticks: {
    //                 suggestedMin: 0,
    //                 suggestedMax: 100
    //             }
    //         }]
    //     }
    // };

    // self.chart_legend = ko.observable(null);

    self.updateChart = function () {
        $.getJSON("/api/sensors/all", function (data) {
            console.log("Sensors", data);

            // add time labels
            var labels = data.time.map(function (x) { return new Date(x * 1000); });
            var datasets = [];

            // add sensor data
            data.sensors.forEach(e => {
                if ('temperature' in e && e.temperature.length > 0) {
                    var d = new Array();
                    e.temperature.forEach((element, index) => {
                        d.push([data.time[index], element]);
                    });
                    datasets.push({
                        label: "Temperature: " + e.name,
                        color: "rgba(220,120,120,1)",
                        highlightColor: "rgba(220,0,0,1)",
                        data: d
                    });
                }
                if ('humidity' in e && e.humidity.length > 0) {
                    var d = new Array();
                    e.humidity.forEach((element, index) => {
                        d.push([data.time[index], element]);
                    });
                    datasets.push({
                        label: "Humidity: " + e.name,
                        color: "rgba(120,120,220,1)",
                        highlightColor: "rgba(0,0,220,1)",
                        data: d
                    });
                }
            });

            // add events
            var events = [];
            var events_info = [];
            data.events.forEach(e => {
                if (e.time >= data.time[0]) {
                    events_info.push({
                        title: e.name,
                        description: e.description
                    });
                    events.push([e.time, 0]);
                }
            });
            datasets.push({
                label: "events",
                color: "rgba(0,220,0,1)",
                points: {
                    show: true
                },
                lines: {
                    show: false
                }, info: events_info,
                data: events,
                // type: "bubble"
            });

            // add thresholds
            data.thresholds.forEach(e => {
                datasets.push({
                    label: "Threshold: " + e.name,
                    color: "rgba(150,150,150, 1)",
                    highlightColor: "rgba(50,50,50,1)",
                    data: [
                        [data.time[0], e.threshold],
                        [data.time[data.time.length - 1], e.threshold],
                    ]
                });
            });

            console.log(datasets);

            var plot = $.plot("#chart", datasets,
                {
                    series: {
                        lines: {
                            show: true
                        },
                        points: {
                            show: false
                        }
                    },
                    grid: {
                        hoverable: true,
                        clickable: false
                    },
                    xaxis: {
                        mode: "time",
                        timeformat: "%H:%M"
                    },
                    yaxis: {
                        position: "left",
                        min: 0,
                        max: 100,
                        autoscaleMargin: 10
                    },
                    zoom: {
                        interactive: false
                    },
                    pan: {
                        interactive: true,
                        enableTouch: true
                    },
                    tooltip: true,
                    tooltipOpts: {
                        content: "%s | X: %x | Y: %y.2",
                        //%s -> series label, %x -> X value, %y -> Y value, %x.2 -> precision of X value
                        dateFormat: "%y-%0m-%0d",
                        shifts: {
                            x: 10,
                            y: 20
                        },
                        // defaultTheme: true
                    },
                    // interaction : {
                    //     redrawOverlayInterval: 1
                    // },
                    // legend: {
                    //     show: false,
                    //     // labelFormatter: null or (fn: string, series object -> string)
                    //     labelBoxBorderColor: "#333333",
                    //     // noColumns: number
                    //     position: "ne",
                    //     // margin: number of pixels or [x margin, y margin]
                    //     // backgroundColor: "transparent",
                    //     backgroundOpacity: 0.5,
                    //     container: document.getElementById("chart-legend")
                    //     // sorted: null/false, true, "ascending", "descending", "reverse", or a comparator
                    // },
                    // comment: {
                    // 	show: true
                    // },
                    // comments: events
                });

            $("<div id='tooltip'></div>").css({
                position: "absolute",
                display: "none",
                border: "1px solid #fdd",
                padding: "2px",
                "background-color": "#fee",
                opacity: 0.80,
                color: "#000"
            }).appendTo("body");

            $("#chart").bind("plothover", function (event, pos, item) {

                if (!pos.x || !pos.y) {
                    return;
                }

                // if(item){
                //     var d = new Date(pos.x*1000).toLocaleTimeString()
                //     var str = "<span style='color:" + item.series.highlightColor + "'><h5>" + item.series.label + "</h5>" + d + " = " + pos.y.toFixed(2) + "</span>";
                //     self.chart_legend(str);
                // }


                // var str = item.series.label + ": " + pos.y.toFixed(2);
                // $("#hoverdata").text(str);

                if (item) {
                    console.log("item", item);

                    var str = "";

                    if (item.series.info !== undefined) {
                        str = "<h5>" + item.series.info[item.dataIndex].title + "</h5>" + item.series.info[item.dataIndex].description

                    } else {

                        var x = new Date(item.datapoint[0] * 1000).toLocaleTimeString();
                        var y = item.datapoint[1].toFixed(2);
                        str = "<h5>" + item.series.label + "</h5>" + x + " = " + y;
                    }

                    $("#tooltip").html(str)
                        .css({ top: item.pageY + 5, left: item.pageX + 5, "background-color": item.series.color })
                        .fadeIn(200);
                } else {
                    $("#tooltip").hide();
                    // self.chart_legend(null);
                }
            });

            $("#chart").bind("plothovercleanup", function (event, pos, item) {
                $("#tooltip").hide();
                // self.chart_legend(null);
            });

            // $("#chart").bind("plotclick", function (event, pos, item) {
            //     if (item) {
            //         $("#clickdata").text(" - click point " + item.dataIndex + " in " + item.series.label);
            //         plot.highlight(item.series, item.datapoint);
            //     }
            // });

            // // add sensor data
            // data.sensors.forEach(e => {
            //     if ('temperature' in e && e.temperature.length > 0) {
            //         datasets.push({
            //             label: "Temperature: " + e.name,
            //             backgroundColor: "rgba(220,0,0,0.2)",
            //             borderColor: "rgba(220,0,0,1)",
            //             pointColor: "rgba(220,0,0,1)",
            //             pointStrokeColor: "#fff",
            //             pointHighlightFill: "#fff",
            //             pointHighlightStroke: "rgba(220,0,0,1)",
            //             data: e.temperature
            //         });
            //     }
            //     if ('humidity' in e && e.humidity.length > 0) {
            //         datasets.push({
            //             label: "Humidity: " + e.name,
            //             backgroundColor: "rgba(0,0,220,0.2)",
            //             borderColor: "rgba(0,0, 220,1)",
            //             pointColor: "rgba(0,0,220,1)",
            //             pointStrokeColor: "#fff",
            //             pointHighlightFill: "#fff",
            //             pointHighlightStroke: "rgba(0,0,220,1)",
            //             data: e.humidity
            //         });
            //     }
            // });

            // // add events
            // var events = [];
            // data.events.forEach(e => {
            //     events.push({
            //         x: new Date(e.time * 1000),
            //         y: 10,
            //         r: 5
            //     });
            // });
            // datasets.push({
            //     label: "events",
            //     backgroundColor: "rgba(0,220,0,0.2)",
            //     borderColor: "rgba(0,220,0,1)",
            //     pointColor: "rgba(0,220,0,1)",
            //     pointStrokeColor: "#fff",
            //     pointHighlightFill: "#fff",
            //     pointHighlightStroke: "rgba(0,255,0,1)",
            //     data: events,
            //     type: "bubble"
            // });

            // // add thresholds
            // data.thresholds.forEach(e => {
            //     datasets.push({
            //         label: "Threshold: " + e.name,
            //         backgroundColor: "rgba(150,150,150,0.2)",
            //         borderColor: "rgba(150,150,150,1)",
            //         pointColor: "rgba(150,150,150,1)",
            //         pointStrokeColor: "#fff",
            //         pointHighlightFill: "#fff",
            //         pointHighlightStroke: "rgba(100,100,100,1)",
            //         data: [
            //             { x: labels[0], y: e.threshold },
            //             { x: labels[labels.length - 1], y: e.threshold },
            //         ]
            //     });
            // });

            // // update chart
            // self.chartData({
            //     labels: labels,
            //     datasets: datasets
            // });
        });
    };


    // Settings
    self.sensor_types = ko.observableArray([
        "ds18b20",
        "SHT31",
        "analog_t",
        "analog_h",
        "DHT11",
        "DHT21",
        "DHT22",
        "AM2301",
        "AM2302",
    ]);

    self.actuator_types = ko.observableArray([
        "toggle",
        "range"
    ]);

    self.comparator_types = ko.observableArray([
        "<",
        ">"
    ]);

    self.sensor_types_th = ko.observableArray([
        "temperature",
        "humidity"
    ]);

    self.tz = [
        { "label": "(GMT-12:00) International Date Line West", "value": "-12" },
        { "label": "(GMT-11:00) Midway Island, Samoa", "value": "-11" },
        { "label": "(GMT-10:00) Hawaii", "value": "-10" },
        { "label": "(GMT-09:00) Alaska", "value": "-9" },
        { "label": "(GMT-08:00) Pacific Time (US & Canada)", "value": "-8" },
        { "label": "(GMT-08:00) Tijuana, Baja California", "value": "-8" },
        { "label": "(GMT-07:00) Arizona", "value": "-7" },
        { "label": "(GMT-07:00) Chihuahua, La Paz, Mazatlan", "value": "-7" },
        { "label": "(GMT-07:00) Mountain Time (US & Canada)", "value": "-7" },
        { "label": "(GMT-06:00) Central America", "value": "-6" },
        { "label": "(GMT-06:00) Central Time (US & Canada)", "value": "-6" },
        { "label": "(GMT-05:00) Bogota, Lima, Quito, Rio Branco", "value": "-5" },
        { "label": "(GMT-05:00) Eastern Time (US & Canada)", "value": "-5" },
        { "label": "(GMT-05:00) Indiana (East)", "value": "-5" },
        { "label": "(GMT-04:00) Atlantic Time (Canada)", "value": "-4" },
        { "label": "(GMT-04:00) Caracas, La Paz", "value": "-4" },
        { "label": "(GMT-04:00) Manaus", "value": "-4" },
        { "label": "(GMT-04:00) Santiago", "value": "-4" },
        { "label": "(GMT-03:30) Newfoundland", "value": "-3.5" },
        { "label": "(GMT-03:00) Brasilia", "value": "-3" },
        { "label": "(GMT-03:00) Buenos Aires, Georgetown", "value": "-3" },
        { "label": "(GMT-03:00) Greenland", "value": "-3" },
        { "label": "(GMT-03:00) Montevideo", "value": "-3" },
        { "label": "(GMT-02:00) Mid-Atlantic", "value": "-2" },
        { "label": "(GMT-01:00) Cape Verde Is.", "value": "-1" },
        { "label": "(GMT-01:00) Azores", "value": "-1" },
        { "label": "(GMT+00:00) Casablanca, Monrovia, Reykjavik", "value": "0" },
        { "label": "(GMT+00:00) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London", "value": "0" },
        { "label": "(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna", "value": "1" },
        { "label": "(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague", "value": "1" },
        { "label": "(GMT+01:00) Brussels, Copenhagen, Madrid, Paris", "value": "1" },
        { "label": "(GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb", "value": "1" },
        { "label": "(GMT+01:00) West Central Africa", "value": "1" },
        { "label": "(GMT+02:00) Amman", "value": "2" },
        { "label": "(GMT+02:00) Athens, Bucharest, Istanbul", "value": "2" },
        { "label": "(GMT+02:00) Beirut", "value": "2" },
        { "label": "(GMT+02:00) Cairo", "value": "2" },
        { "label": "(GMT+02:00) Harare, Pretoria", "value": "2" },
        { "label": "(GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius", "value": "2" },
        { "label": "(GMT+02:00) Jerusalem", "value": "2" },
        { "label": "(GMT+02:00) Minsk", "value": "2" },
        { "label": "(GMT+02:00) Windhoek", "value": "2" },
        { "label": "(GMT+03:00) Kuwait, Riyadh, Baghdad", "value": "3" },
        { "label": "(GMT+03:00) Moscow, St. Petersburg, Volgograd", "value": "3" },
        { "label": "(GMT+03:00) Nairobi", "value": "3" },
        { "label": "(GMT+03:00) Tbilisi", "value": "3" },
        { "label": "(GMT+03:30) Tehran", "value": "3.5" },
        { "label": "(GMT+04:00) Abu Dhabi, Muscat", "value": "4" },
        { "label": "(GMT+04:00) Baku", "value": "4" },
        { "label": "(GMT+04:00) Yerevan", "value": "4" },
        { "label": "(GMT+04:30) Kabul", "value": "4.5" },
        { "label": "(GMT+05:00) Yekaterinburg", "value": "5" },
        { "label": "(GMT+05:00) Islamabad, Karachi, Tashkent", "value": "5" },
        { "label": "(GMT+05:30) Sri Jayawardenapura", "value": "5.5" },
        { "label": "(GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi", "value": "5.5" },
        { "label": "(GMT+05:45) Kathmandu", "value": "5.75" },
        { "label": "(GMT+06:00) Almaty, Novosibirsk", "value": "6" }, { "label": "(GMT+06:00) Astana, Dhaka", "value": "6" },
        { "label": "(GMT+06:30) Yangon (Rangoon)", "value": "6.5" },
        { "label": "(GMT+07:00) Bangkok, Hanoi, Jakarta", "value": "7" },
        { "label": "(GMT+07:00) Krasnoyarsk", "value": "7" },
        { "label": "(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi", "value": "8" },
        { "label": "(GMT+08:00) Kuala Lumpur, Singapore", "value": "8" },
        { "label": "(GMT+08:00) Irkutsk, Ulaan Bataar", "value": "8" },
        { "label": "(GMT+08:00) Perth", "value": "8" },
        { "label": "(GMT+08:00) Taipei", "value": "8" },
        { "label": "(GMT+09:00) Osaka, Sapporo, Tokyo", "value": "9" },
        { "label": "(GMT+09:00) Seoul", "value": "9" },
        { "label": "(GMT+09:00) Yakutsk", "value": "9" },
        { "label": "(GMT+09:30) Adelaide", "value": "9.5" },
        { "label": "(GMT+09:30) Darwin", "value": "9.5" },
        { "label": "(GMT+10:00) Brisbane", "value": "10" },
        { "label": "(GMT+10:00) Canberra, Melbourne, Sydney", "value": "10" },
        { "label": "(GMT+10:00) Hobart", "value": "10" },
        { "label": "(GMT+10:00) Guam, Port Moresby", "value": "10" },
        { "label": "(GMT+10:00) Vladivostok", "value": "10" },
        { "label": "(GMT+11:00) Magadan, Solomon Is., New Caledonia", "value": "11" },
        { "label": "(GMT+12:00) Auckland, Wellington", "value": "12" },
        { "label": "(GMT+12:00) Fiji, Kamchatka, Marshall Is.", "value": "12" },
        { "label": "(GMT+13:00) Nuku'alofa", "value": "13" }
    ];


    self.settings = ko.observable(new Settings());
    $.getJSON("/api/config", function (data) {
        console.log("config", data);
        var general = new SettingsGeneral(data.general.gmt_offset, data.general.dst);
        var network = new SettingsNetwork(data.network.hostname);
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
            timers.push(new SettingsTimer(e.name, actuators.find(a => a.name() == e.actuator), e.on, e.off, e.inverted));
        });

        // add sensors
        data.sensors.forEach(e => {
            sensors.push(new SettingsSensor(e.name, e.type,
                e.gpio, e.address, e.enabled));
        });


        // add thresholds
        data.thresholds.forEach(e => {
            thresholds.push(new SettingsThreshold(e.name, e.duration, e.threshold, e.comparator,
                actuators.find(a => a.name() == e.actuator), e.inverted, e.gap,
                sensors.find(a => a.name() == e.sensor), e.sensor_type));
        });

        var settings = new Settings(general, network, timers, sensors, actuators, thresholds);
        console.log("Settings: ", ko.toJSON(settings));
        self.settings(settings);
    });

    self.save_settings = function () {
        var data = self.settings().flatten();

        console.log(ko.toJSON(data, null, "\t"));

        var jqxhr = $.post("/api/config",
            ko.toJSON(data),
            function () {
                console.log("sent!");
            })
            .done(function (resp) {
                console.log(resp);
                alert("Saved config! Will reboot now.");
                location.reload(); 
            })
            .fail(function (e) {
                console.log(e);
                if(e.responseText !== undefined){
                    alert("Error: " + e.responseText);
                } else {
                    location.reload();
                }
            });
    };

    self.addSensor = function () {
        self.settings().sensors.push(new SettingsSensor("new sensor",
            self.sensor_types[0],
            15,
            68,
            false));
    };

    self.addActuator = function () {
        self.settings().actuators.push(new SettingsActuator("new actuator",
            12,
            self.actuator_types[0],
            0,
            1023));
    };

    self.addTimer = function () {
        self.settings().timers.push(new SettingsTimer("new timer",
            self.settings().actuators[0],
            "00:00",
            "01:00",
            false));
    };

    self.addThreshold = function () {
        self.settings().thresholds.push(new SettingsThreshold("new threshold",
            0,
            0,
            self.comparator_types[0],
            self.settings().actuators[0],
            false,
            60,
            self.settings().sensors[0],
            self.sensor_types_th[0])
        );
    };

    self.removeSensor = function (sensor) {
        self.settings().sensors.remove(sensor);
    };

    self.removeActuator = function (actuator) {
        self.settings().actuators.remove(actuator);
    };

    self.removeTimer = function (timer) {
        self.settings().timers.remove(timer);
    };

    self.removeThreshold = function (threshold) {
        self.settings().thresholds.remove(threshold);
    };
}

var model = new TerraESPViewModel();
window.setInterval(model.updateChart, 60000);
model.updateChart();
ko.applyBindings(model);