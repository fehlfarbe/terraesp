(function (factory) {
    // Module systems magic dance.

    if (typeof require === "function" && typeof exports === "object" && typeof module === "object") {
        // CommonJS or Node: hard-coded dependency on "knockout"
        factory(require("knockout"), require("chart"), exports);
    } else if (typeof define === "function" && define["amd"]) {
        // AMD anonymous module with hard-coded dependency on "knockout"
        define(["knockout", "chart", "exports"], factory);
    } else {
        // <script> tag: use the global `ko` object, attaching a `mapping` property
        factory(ko, Chart);
    }
}
(function (ko, Chart, exports) {

    ko.observableGroup = function(observables) {
        var observableManager = {};
        var throttle = 0;
        var throttleTimeout;

        observableManager.throttle = function(duration) {
            throttle = duration;
            return observableManager;
        };

        observableManager.subscribe = function(handler) {
            function throttledHandler(val) {
                if(throttle > 0) {
                    if(!throttleTimeout) {
                        throttleTimeout = setTimeout(function() {
                            throttleTimeout = undefined;
                            handler(val);
                        }, throttle);
                    }
                }
                else
                { handler(val); }
            }

            for(var i = 0; i < observables.length; i++)
            { observables[i].subscribe(throttledHandler); }

            return observableManager;
        };

        return observableManager;
    };

    var getType = function(obj) {
        if ((obj) && (typeof (obj) === "object") && (obj.constructor == (new Date).constructor)) return "date";
        return typeof obj;
    };

    var getSubscribables = function(model) {
        var subscribables = [];
        scanForObservablesIn(model, subscribables);
        return subscribables;
    };

        var scanForObservablesIn = function (model, subscribables) {
            if (model === null || model === undefined) {
                return;
            }

            var propertyNames = [];
            if (window.navigator.userAgent.indexOf("MSIE") > -1 || navigator.userAgent.indexOf("Trident") > -1) {
                propertyNames = Object.getOwnPropertyNames(model);
            }
            else {
                propertyNames = Reflect.ownKeys(model);
            }

            propertyNames.forEach(function (propertyName) {
                var typeOfData = getType(model[propertyName]);
                switch (typeOfData) {
                    case "object": { scanForObservablesIn(model[propertyName], subscribables); } break;
                    case "array":
                        {
                            var underlyingArray = model[propertyName]();
                            underlyingArray.forEach(function (entry, index) { scanForObservablesIn(underlyingArray[index], subscribables); });
                        }
                        break;

                    default:
                        {
                            if (ko.isComputed(model[propertyName]) || ko.isObservable(model[propertyName])) { subscribables.push(model[propertyName]); }
                        }
                        break;
                }
            });
        };

    ko.bindingHandlers.chart = {
        init: function (element, valueAccessor, allBindingsAccessor, viewModel) {
            var allBindings = allBindingsAccessor();
            var chartBinding = allBindings.chart;
            var activeChart;
            var chartData;

            var createChart = function() {
                var chartType = ko.unwrap(chartBinding.type);
                var data = ko.toJS(chartBinding.data);
                var options = ko.toJS(chartBinding.options);

                chartData = {
                    type: chartType,
                    data: data,
                    options: options
                };

                activeChart = new Chart(element, chartData);
            };

            var refreshChart = function() {
                chartData.data = ko.toJS(chartBinding.data);
                activeChart.update();
                activeChart.resize();
            };

            var subscribeToChanges = function() {
                var throttleAmount = ko.unwrap(chartBinding.options.throttle) || 100;
                var dataSubscribables = getSubscribables(chartBinding.data);
                console.log("found obs", dataSubscribables);

                ko.observableGroup(dataSubscribables)
                    .throttle(throttleAmount)
                    .subscribe(refreshChart);
            };

            createChart();

            if(chartBinding.options && chartBinding.options.observeChanges)
            { subscribeToChanges(); }
        }
    };

}));