var xicChart = null;
var xicGraphId = graphExporter.xicGraphId;

var massPeakChart = null;
var massPeakGraphId = graphExporter.massPeakGraphId;

var lastPlotData = {};

var imageFormats = graphExporter.supportedImageFormatIds;
var dataFormats = graphExporter.supportedDataFormatIds;

var fakePointKey = "fake_data";

function getChartById(id) {
    switch (id) {
        case xicGraphId:
            return xicChart;
        case massPeakGraphId:
            return massPeakChart;
    }
}

function clone(obj) {
    if (null == obj || 'object' != typeof obj) return obj;
    var copy = obj.constructor();
    for (var attr in obj) {
        copy[attr] = obj[attr];
    }
    return copy;
}

function isEmpty(obj) {
    for(var prop in obj) {
        if(obj.hasOwnProperty(prop))
            return false;
    }
    return true && JSON.stringify(obj) === JSON.stringify({});
}

function getCoordinates(chart) {
    var xCoordKeys = [];
    var yCoordKeys = [];
    for (var i = 0; i < chart.graphs.length; ++i) {
        var graph = chart.graphs[i];
        xCoordKeys.push(graph.xField);
        yCoordKeys.push(graph.yField);
    }

    var resultPoints = [];
    for (var i = 0; i < chart.dataProvider.length; ++i) {
        var originalPoint = chart.dataProvider[i];
        var resultPoint = {};
        if (fakePointKey in originalPoint) {
            continue;
        }
        for (var key in originalPoint) {
            if (xCoordKeys.indexOf(key) != -1) {
                resultPoint['x'] = key;
                resultPoint[key] = originalPoint[key];
            } else if (yCoordKeys.indexOf(key) != -1) {
                var delimeter = '; ';
                var re = new RegExp('<br>', 'g');
                var yFieldName = key.replace(re, delimeter);
                if (yFieldName.substr(yFieldName.length - delimeter.length) == delimeter) {
                    yFieldName = yFieldName.slice(0, -delimeter.length);
                }
                if ('precursorMz' in originalPoint) {
                    yFieldName = ('Precursor mz: ' + originalPoint['precursorMz'].toString()).concat(delimeter, yFieldName);
                }
                resultPoint['y'] = yFieldName;
                resultPoint[yFieldName] = originalPoint[key];
            }
        }
        resultPoints.push(resultPoint);
    }
    return resultPoints;
}

function generateFormatListMenu(graphId, formats) {
    var menuItems = [];
    for (var i = 0; i < formats.length; ++i) {
        var formatId = formats[i];
        var menuItem = {
            'label': formatId,
            'click': function (event, menuItem) {
                var chart = getChartById(graphId);

                // seems that chart.chartCursor.enabled property doesn't work so we play with alpha of cursor
                // TODO: check the property with future versions of amCharts
                var cursorAlpha = chart.chartCursor.cursorAlpha;
                chart.chartCursor.cursorAlpha = 0;
                chart.chartCursor.valueLineBalloonEnabled = false;

                chart.chartScrollbar.enabled = false;
                chart.validateData();

                graphExporter.exportGraph(graphId, menuItem.label, getCoordinates(chart));

                chart.chartScrollbar.enabled = true;
                chart.chartCursor.cursorAlpha = cursorAlpha;
                chart.chartCursor.valueLineBalloonEnabled = true;
                chart.validateData();
            }
        };
        menuItems.push(menuItem);
    }
    return menuItems;
}

function generatePlotExportDescriptor(graphId) {
    return {
        'enabled': true,
        'menu': [{
            'class': 'export-main',
            'menu': [{
                'label': 'Save image...',
                'menu': generateFormatListMenu(graphId, imageFormats)
            }, {
                'label': 'Save plot data...',
                'menu': generateFormatListMenu(graphId, dataFormats)
            }]
        }]
    };
}

function generatePlotScrollbarDescriptor() {
    return {
        'dragIconHeight': 20
    };
}

// Source: http://stackoverflow.com/questions/11832914/round-to-at-most-2-decimal-places-in-javascript#answer-19722641
Number.prototype.round = function (places) {
    return +(Math.round(this + 'e+' + places) + 'e-' + places);
}

function getXicGraphProto(baloonMsg) {
    return {
        'balloonFunction': adjustXicBalloonText,
        'bulletField': 'bullet',
        'bulletBorderAlpha': 1,
        'colorField': 'color',
        'bulletSizeField': 'bulletSize',
        'fillAlphas': 1,
        'hideBulletsCount': 1000,
    };
}

function adjustXicBalloonText(graphDataItem, graph) {
    var hasPrecursorMz = 'precursorMz' in graph.chart.dataProvider[graphDataItem.index];
    return graph.yField
        + (hasPrecursorMz ? ('Precursor mz: ' + graph.chart.dataProvider[graphDataItem.index]['precursorMz'].round(4) + '<br>(Click to see MS/MS spectrum)<br>') : '')
        + '<b><span style="font-size:14px;">RT:' + graphDataItem.values.x.round(2) + ' s</span></b>';
}

function getIsotopicPatternGraphProto(baloonMsg) {
    return {
        'balloonFunction': adjustMassPeakBalloonText,
        'lineAlpha': 1,
        'fillAlphas': 1,
        'bulletField': 'bullet',
        'bulletSizeField': 'bulletSize',
        'bulletBorderAlpha': 1,
        'useLineColorForBulletBorder': true,
        'hideBulletsCount': 1000,
    };
}

function adjustMassPeakBalloonText(graphDataItem, graph) {
    var hasScanStartTime = 'scanStartTime' in graph.chart.dataProvider[graphDataItem.index];
    var hasPrecursorMz = 'precursorMz' in graph.chart.dataProvider[graphDataItem.index];
    return graph.yField
        + (hasPrecursorMz ? ('Precursor mz: ' + graph.chart.dataProvider[graphDataItem.index]['precursorMz'].round(4) + '<br>') : '')
        + (hasScanStartTime ? ('Scan start time: ' + graph.chart.dataProvider[graphDataItem.index]['scanStartTime'].round(2) + ' s<br>') : '')
        + '<b><span style="font-size:14px;">mz:' + graphDataItem.values.x.round(4) + '</span></b>';
}

function getGraphs(data, horAxisDataId, graphProtoBuilder) {
    var graphs = [];
    var usedKeys = {};
    for (var i = 0; i < data.length; ++i) {
        for (var xKey in data[i]) {
            if (xKey.slice(0, horAxisDataId.length) != horAxisDataId) {
                continue;
            }
            var yKey;
            for (var key in data[i]) {
                if (key != xKey && key.indexOf('Sample:') > -1) { // TODO: refactor
                    yKey = key;
                    break;
                }
            }
            var firstGraphPoint = false;
            if (!(xKey in usedKeys)) {
                var currentGraph = graphProtoBuilder(yKey);
                currentGraph['xField'] = xKey;
                currentGraph['yField'] = yKey;
                usedKeys[xKey] = null;
                graphs.push(currentGraph);
                firstGraphPoint = true;
            }

            if (horAxisDataId == 'rt_') { // TODO: refactor
                if ('precursorMz' in data[i]) {
                    data[i]['bullet'] = 'round';
                    data[i]['color'] = '#B22222';
                    data[i]['bulletSize'] = 10;
                    data[i][fakePointKey] = true;
                } else {
                    data[i]['bullet'] = 'round';
                    data[i]['color'] = '#A8A8A8';
                    data[i]['bulletSize'] = 0.5;
                }
                if (firstGraphPoint) {
                    var boundaryPoint = clone(data[i]);
                    boundaryPoint[yKey] = 0.0;
                    boundaryPoint[fakePointKey] = true;
                    data.splice(i++, 0, boundaryPoint);
                }
            }

            if (horAxisDataId == 'mz_') { // TODO: refactor
                var precedingPoint = clone(data[i]);
                precedingPoint[yKey] = 0.0;
                precedingPoint[fakePointKey] = true;

                var successivePoint = clone(data[i]);
                successivePoint[yKey] = 0.0;
                successivePoint[fakePointKey] = true;

                data[i]['bullet'] = 'round';
                data[i]['bulletSize'] = 0.5;

                data.splice(i++, 0, precedingPoint);
                data.splice(++i, 0, successivePoint);
            }
        }
    }

    if (horAxisDataId == 'rt_') { // TODO: refactor
        for (var i = data.length - 1; !isEmpty(usedKeys) && i >= 0; --i) {
            var lastGraphPoint = false;
            var foundKey = null;
            for (var key in data[i]) {
                if (key in usedKeys) {
                    lastGraphPoint = true;
                    foundKey = key;
                    break;
                }
            }
            if (lastGraphPoint) {
                var yKey;
                for (var key in data[i]) {
                    if (key != foundKey && key.indexOf('Sample:') > -1) { // TODO: refactor
                        yKey = key;
                        break;
                    }
                }

                var boundaryPoint = clone(data[i]);
                boundaryPoint[yKey] = 0.0;
                boundaryPoint[fakePointKey] = true;
                data.splice(i + 1, 0, boundaryPoint);
                delete usedKeys[foundKey];
            }
        }
    }
    return graphs;
}

var xicGraphSelectionState = {
    _selectedItems: [],
    _selectedCharts: [],
    _alphasByGraph: [],
    _lastClickEventOffX: 0,
    _lastClickEventOffY: 0,
    _selectionActive: false,

    reset: function() {
        this._selectedItems = [];
        this._selectedCharts = [];
        this._alphasByGraph = [];
        this._lastClickEventOffX = 0;
        this._lastClickEventOffY = 0;
        this._selectionActive = false;
    },

    deselect: function(event) {
        if (this._selectionActive && !(this._lastClickEventOffX == event.offsetX && this._lastClickEventOffY == event.offsetY)) {
            for (var i = 0; i < this._alphasByGraph.length; ++i) {
                this._alphasByGraph[i]['graph'].fillAlphas = this._alphasByGraph[i]['fillAlphas'];
                this._alphasByGraph[i]['graph'].lineAlpha = this._alphasByGraph[i]['lineAlpha'];
            }
            this._alphasByGraph = [];
            for (var i = 0; i < this._selectedItems.length; ++i) {
                this._selectedItems[i]['item'].dataContext[this._selectedItems[i]['colorField']] = this._selectedItems[i]['previousColor'];
            }
            this._selectedItems = [];

            for (var i = 0; i < this._selectedCharts.length; ++i) {
                this._selectedCharts[i].validateData();
            }
            this._selectedCharts = [];

            this._selectionActive = false;

            updateMassPeakChartData(lastPlotData['isotopicPattern']);
        }
    },

    select: function (event) {
        event.event.stopPropagation();

        var isMs2ScanClicked = 'precursorMz' in event.graph.chart.dataProvider[event.item.index];
        if (!event.event.ctrlKey || !isMs2ScanClicked) {
            this.deselect(event.event);
        }
        if (!isMs2ScanClicked) {
            return;
        }

        if (event.event.ctrlKey) {
            for (var i = 0; i < this._selectedItems.length; ++i) {
                if (this._selectedItems[i]['item'].x == event.item.x && this._selectedItems[i]['item'].y == event.item.y) {
                    if (this._selectedItems.length == 1) {
                        this.deselect(event.event);
                        return;
                    } else {
                        this._selectedItems[i]['item'].dataContext[this._selectedItems[i]['colorField']] = this._selectedItems[i]['previousColor'];
                        this._selectedItems.splice(i, 1);
                        event.chart.validateData();
                        this._updateMs2Spectra();
                        return;
                    }
                }
            }
        }

        this._selectedItems.push({
            'item': event.item,
            'colorField': event.graph.colorField,
            'previousColor': event.item.dataContext[event.graph.colorField]
        });

        if (-1 == this._selectedCharts.indexOf(event.chart)) {
            this._selectedCharts.push(event.chart);

            this._alphasByGraph = [];
            for (var i = 0; i < event.chart.graphs.length; ++i) {
                this._alphasByGraph.push({
                    'graph': event.chart.graphs[i],
                    'fillAlphas': event.chart.graphs[i]['fillAlphas'],
                    'lineAlpha': event.chart.graphs[i]['lineAlpha']
                });
                event.chart.graphs[i].fillAlphas = 0.1;
                event.chart.graphs[i].lineAlpha = 0.1;
            }

            this._selectionActive = true;
        }

        this._lastClickEventOffX = event.event.offsetX;
        this._lastClickEventOffY = event.event.offsetY;

        event.item.dataContext[event.graph.colorField] = '#FFD700';
        event.chart.validateData();

        this._updateMs2Spectra();
    },

    _updateMs2Spectra: function() {
        var scanTimesByScanKey = {};
        for (var i = 0; i < this._selectedItems.length; ++i) {
            var selectedItem = this._selectedItems[i];
            var scanKey = '';
            for (var key in selectedItem.item.dataContext) {
                if (key.indexOf('rt_') > -1) { // TODO: refactor
                    scanKey = key;
                    break;
                }
            }

            if (!(scanKey in scanTimesByScanKey)) {
                var intKey = '';
                for (var key in selectedItem.item.dataContext) {
                    if (key.indexOf('Sample:') > -1) { // TODO: refactor
                        intKey = key;
                        break;
                    }
                }

                var scansForFeatureSample = {
                    'scanKey': scanKey,
                    'intKey': intKey,
                    'scanTimes': [selectedItem.item.dataContext[scanKey]]
                };
                scanTimesByScanKey[scanKey.toString()] = scansForFeatureSample;
            } else {
                scanTimesByScanKey[scanKey.toString()]['scanTimes'].push(selectedItem.item.dataContext[scanKey]);
            }
        }

        var values = [];
        for (var key in scanTimesByScanKey) {
            values.push(scanTimesByScanKey[key]);
        }
        var graphPoints = dataController.getMs2Spectra(values);

        updateMassPeakChartData(graphPoints);
    }
};

function createXicChart(div_id, dataProvider, graphs) {
    return AmCharts.makeChart(div_id, {
        'type': 'xy',
        'theme': 'patterns',
        'autoMarginOffset': 20,
        'startDuration': 0.1,
        'dataProvider': dataProvider,
        'graphs': graphs,
        'valueAxes': [{
            'id': 'x',
            'position': 'bottom',
            'dashLength': 1,
            'title': 'Retention time [s]'
        }, {
            'id': 'y',
            'dashLength': 1,
            'position': 'left',
            'title': 'Intensity [number of ions]'
        }],
        'chartScrollbar': generatePlotScrollbarDescriptor(),
        'chartCursor': {
            'bulletsEnabled': true,
            'valueLineAxis': 'y',
            'valueLineBalloonEnabled': true
        },
        'marginLeft': 60,
        'marginBottom': 60,
        'marginRight': 60,
        'export': generatePlotExportDescriptor(xicGraphId),
        'responsive': {
            'enabled': true
        },
        'listeners': [{
            'event': 'clickGraphItem',
            'method': function(event) {
                xicGraphSelectionState.select(event);
            }
        }]
    });
}

document.getElementById('xic_div').addEventListener('click', function (event) {
    xicGraphSelectionState.deselect(event);
});

function createMassPeakChart(div_id, dataProvider, graphs) {
    var result = AmCharts.makeChart(div_id, {
        'type': 'xy',
        'marginLeft': 60,
        'marginBottom': 60,
        'marginRight': 60,
        'autoMarginOffset': 20,
        'startDuration': 0.1,
        'dataProvider': dataProvider,
        'graphs': graphs,
        'chartScrollbar': generatePlotScrollbarDescriptor(),
        'chartCursor': {
            'valueLineAxis': 'y',
            'valueLineBalloonEnabled': true,
        },
        'valueAxes': [{
            'id': 'x',
            'position': 'bottom',
            'dashLength': 1,
            'title': 'mz'
        }, {
            'id': 'y',
            'dashLength': 1,
            'position': 'left',
            'title': 'Intensity [number of ions]'
        }],
        'export': generatePlotExportDescriptor(massPeakGraphId),
        'responsive': {
            'enabled': true
        }
    });
    return result;
}

function updateMassPeakChartData(data) {
    var massPeakGraphs = getGraphs(data, 'mz_', getIsotopicPatternGraphProto);

    if (null != massPeakChart) {
        massPeakChart.clear();
    }

    massPeakChart = createMassPeakChart('mass_peak_div', data, massPeakGraphs);
}

function updateChartData(data) {
    xicGraphSelectionState.reset();

    var xicGraphs = getGraphs(data['xic'], 'rt_', getXicGraphProto);

    if (null != xicChart) {
        xicChart.clear();
    }
    xicChart = createXicChart('xic_div', data['xic'], xicGraphs);

    updateMassPeakChartData(data['isotopicPattern']);

    lastPlotData = data;
}

dataController.updatePlot.connect(this, updateChartData);
