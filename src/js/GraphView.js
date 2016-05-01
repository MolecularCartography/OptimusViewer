var chartsById = {};
chartsById[graphExporter.xicChartId] = null;
chartsById[graphExporter.massPeakChartId] = null;

var imageFormats = graphExporter.supportedImageFormatIds;
var dataFormats = graphExporter.supportedDataFormatIds;

var auxPointFlag = "aux_data";

var actualPlotData = {};

function clone(obj) {
    if (null == obj || 'object' != typeof obj) return obj;
    var copy = obj.constructor();
    for (var attr in obj) {
        copy[attr] = obj[attr];
    }
    return copy;
}

function getCoordinates(chart) {
    var xCoordKeys = [];
    var yCoordKeys = [];
    for (var i = 0; i < chart.graphs.length; ++i) {
        var graph = chart.graphs[i];
        if (!graph.hidden) {
            xCoordKeys.push(graph.xField);
            yCoordKeys.push(graph.yField);
        }
    }

    var resultPoints = [];
    for (var i = 0; i < chart.dataProvider.length; ++i) {
        var originalPoint = chart.dataProvider[i];
        var resultPoint = {};
        if (auxPointFlag in originalPoint) {
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
                if (dataController.precursorMzKey in originalPoint) {
                    yFieldName = ('Precursor m/z: ' + originalPoint[dataController.precursorMzKey].toString()).concat(delimeter, yFieldName);
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
                var chart = chartsById[graphId];

                // seems that chart.chartCursor.enabled property doesn't work so we play with alpha of cursor
                // TODO: check the property with future versions of amCharts
                var cursorAlpha = chart.chartCursor.cursorAlpha;
                chart.chartCursor.cursorAlpha = 0;
                var zoomButtonImage = chart.zoomOutButtonImage;
                chart.zoomOutButtonImage = '';
                var zoomButtonText = chart.zoomOutText;
                chart.zoomOutText = '';
                chart.chartCursor.valueLineBalloonEnabled = false;

                chart.chartScrollbar.enabled = false;
                chart.validateData();

                graphExporter.exportGraph(graphId, menuItem.label, getCoordinates(chart));

                chart.chartScrollbar.enabled = true;
                chart.chartCursor.cursorAlpha = cursorAlpha;
                chart.zoomOutButtonImage = zoomButtonImage;
                chart.zoomOutText = zoomButtonText;
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

function generateXicGraphProto() {
    return {
        'balloonFunction': adjustXicBalloonText,
        'bulletField': 'bullet',
        'bulletBorderAlpha': 1,
        'colorField': 'color',
        'bulletSizeField': 'bulletSize',
        'fillAlphas': 0.7,
        'hideBulletsCount': 1000
    };
}

function adjustXicBalloonText(graphDataItem, graph) {
    var hasPrecursorMz = dataController.precursorMzKey in graph.chart.dataProvider[graphDataItem.index];
    return (hasPrecursorMz ? ('Precursor m/z: ' + graph.chart.dataProvider[graphDataItem.index][dataController.precursorMzKey].round(4)
        + '<br>(Click to see MS/MS spectrum)<br>') : '')
        + '<b><span style="font-size:14px;">Scan start time:' + graphDataItem.values.x.round(2) + ' s</span></b>';
}

function generateMassGraphProto() {
    return {
        'balloonFunction': adjustMassPeakBalloonText,
        'lineAlpha': 1,
        'fillAlphas': 1,
        'bulletField': 'bullet',
        'bulletSizeField': 'bulletSize',
        'bulletBorderAlpha': 1,
        'useLineColorForBulletBorder': true,
        'hideBulletsCount': 1000
    };
}

function adjustMassPeakBalloonText(graphDataItem, graph) {
    return '<b><span style="font-size:14px;">m/z:' + graphDataItem.values.x.round(4) + '</span></b>';
}

function xicPointAttributeSetter(point) {
    if (dataController.precursorMzKey in point) {
        point['bullet'] = 'round';
        point['color'] = '#B22222';
        point['bulletSize'] = 10;
        point[auxPointFlag] = true;
    }
}

function massPointAttributeSetter(point) {
    point['bullet'] = 'round';
    point['bulletSize'] = 0.5;
}

function createAuxGroundPoint(point, valueAxisKey, categoryAxisKey, categoryOffset) {
    var auxPoint = clone(point);
    auxPoint[categoryAxisKey] += categoryOffset;
    auxPoint[valueAxisKey] = 0.0;
    auxPoint[auxPointFlag] = true;
    return auxPoint;
}

function isAuxPoint(point) {
    return auxPointFlag in point;
}

function getGraphs(graphDescriptors, points, graphProtoGenerator, horizontalOffset, stickPlot, pointAttributeSetter) {
    var graphs = [];

    if (!graphDescriptors || !points) {
        return graphs;
    }

    var lastGraphId = null;
    for (var i = 0; i < points.length; ++i) {
        var curPoint = points[i];
        var curGraphId = curPoint[dataController.graphIdKey];
        var curGraphDescriptor = graphDescriptors[curGraphId];

        if (isAuxPoint(curPoint)) {
            continue;
        }

        // add auxilary ground points at ends of current graph
        var addAuxBefore = stickPlot || curGraphId !== lastGraphId;
        var addAuxAfter = stickPlot || i === points.length - 1;

        var addOffsetBefore = horizontalOffset > 0 && curGraphId !== lastGraphId;
        var addOffsetAfter = horizontalOffset > 0 && i === points.length - 1;

        if (curGraphId !== lastGraphId) {
            var currentGraph = graphProtoGenerator();
            currentGraph['xField'] = curGraphDescriptor[dataController.xFieldKey];
            currentGraph['yField'] = curGraphDescriptor[dataController.yFieldKey];
            currentGraph['title'] = 'Sample: ' + curGraphDescriptor[dataController.sampleNameGraphKey];
            if (dataController.scanIdKey in curGraphDescriptor) {
                currentGraph['title'] += '<br>Scan ID: ' + curGraphDescriptor[dataController.scanIdKey];
            } else if (dataController.consensusMzGraphKey in curGraphDescriptor) {
                currentGraph['title'] += '<br>Consensus m/z: ' + curGraphDescriptor[dataController.consensusMzGraphKey].round(4);
            }
            currentGraph['id'] = curGraphId;
            graphs.push(currentGraph);
            lastGraphId = curGraphId;
        }

        var xField = curGraphDescriptor[dataController.xFieldKey];
        var yField = curGraphDescriptor[dataController.yFieldKey];
        if (addOffsetBefore) {
            points.splice(i++, 0, createAuxGroundPoint(curPoint, yField, xField, -horizontalOffset));
        }
        if (addAuxBefore && (i === 0 || !(isAuxPoint(points[i - 1]) && points[i - 1][xField] === curPoint[xField]))) {
            points.splice(i++, 0, createAuxGroundPoint(curPoint, yField, xField, 0));
        }
        if (addAuxAfter && (i === points.length - 1 || !(isAuxPoint(points[i + 1]) && points[i + 1][xField] === curPoint[xField]))) {
            points.splice(++i, 0, createAuxGroundPoint(curPoint, yField, xField, 0));
        }
        if (addOffsetAfter) {
            points.splice(++i, 0, createAuxGroundPoint(curPoint, yField, xField, horizontalOffset));
        }
        pointAttributeSetter(curPoint);
    }
    return graphs;
}

var xicGraphSelectionState = {
    _selectedItems: [],
    _selectedCharts: [],
    _alphasByGraph: [],
    _selectionActive: false,

    reset: function() {
        this._selectedItems = [];
        this._selectedCharts = [];
        this._alphasByGraph = [];
        this._selectionActive = false;
    },

    deselect: function(event) {
        if (this._selectionActive) {
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

            updateMassChartData(actualPlotData[dataController.ms1GraphDescKey], actualPlotData[dataController.ms1GraphDataKey], false);
        }
    },

    select: function (event) {
        event.event.stopPropagation();

        if (!event.event.ctrlKey) {
            this.deselect(event.event);
        }
        var isMs2ScanClicked = dataController.precursorMzKey in event.graph.chart.dataProvider[event.item.index];
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
                event.chart.graphs[i].fillAlphas = 0.3;
                event.chart.graphs[i].lineAlpha = 0.3;
            }

            this._selectionActive = true;
        }

        event.item.dataContext[event.graph.colorField] = '#FFD700';
        event.chart.validateData();

        this._updateMs2Spectra();
    },

    _updateMs2Spectra: function() {
        var spectraIds = this._selectedItems.map(function(item) {
            return item.item.dataContext[dataController.spectrumIdKey];
        });
        var graphData = dataController.getMs2Spectra(spectraIds);
        var graphDescriptors = graphData[dataController.msnGraphDescKey];
        for (var graphId in graphDescriptors) {
            var xicPoint = null;
            for (var i = 0; i < this._selectedItems.length; ++i) {
                if (this._selectedItems[i].item.dataContext[dataController.spectrumIdKey] == graphId) {
                    xicPoint = this._selectedItems[i].item.dataContext;
                    break;
                }
            }
            if (null === xicPoint) {
                throw 'Failed to find XIC point with fragmentation spectrum ID ' + graphId;
            }

            graphDescriptors[graphId][dataController.scanIdKey] = xicPoint[dataController.scanIdKey];
            graphDescriptors[graphId][dataController.sampleNameGraphKey]
                = actualPlotData[dataController.xicGraphDescKey][xicPoint[dataController.graphIdKey]][dataController.sampleNameGraphKey];
        }
        updateMassChartData(graphDescriptors, graphData[dataController.msnGraphDataKey], true);
    }
};

var colors = ["#FF6600", "#FCD202", "#B0DE09", "#0D8ECF", "#2A0CD0", "#CD0D74", "#CC0000", "#00CC00", "#0000CC", "#DDDDDD", "#999999", "#333333", "#990000"];

function createXicGuides(graphs, graphDescriptors) {
    var guides = [];
    for (var i = 0; i < graphs.length; ++i) {
        var descriptor = graphDescriptors[graphs[i]['id']];
        guides.push({
            value: descriptor[dataController.featureStartGraphKey],
            toValue: descriptor[dataController.featureEndGraphKey],
            above: false,
            lineAlpha: 1,
            fillAlpha: 0.1,
            dashLength: 10,
            fillColor: colors[i],
            lineColor: colors[i]
        });
    }
    return guides;
}

function toggleAllGraphs(item, action) {
    for (var chartId in chartsById) {
        var chart = chartsById[chartId];
        if (chart == item.chart) {
            var chartGuides = chart['valueAxes'][0]['guides'];
            if (action == 'hide') {
                chartGuides[item.dataItem.index].lineAlpha = 0;
                chartGuides[item.dataItem.index].fillAlpha = 0;
            } else {
                chartGuides[item.dataItem.index].lineAlpha = 1;
                chartGuides[item.dataItem.index].fillAlpha = 0.1;
            }
            chart.validateData();
        } else if (chart.graphs[item.dataItem.index]['title'] === item.chart[item.dataItem.index]['title']) {
            if (action == 'hide') {
                chart.hideGraph(chart.graphs[item.dataItem.index]);
            } else {
                chart.showGraph(chart.graphs[item.dataItem.index]);
            }
        }
    }
}

function createXicChart(div_id, dataProvider, graphs, guides) {
    return AmCharts.makeChart(div_id, {
        'type': 'xy',
        'theme': 'light',
        'autoMarginOffset': 20,
        'startDuration': 0.1,
        'dataProvider': dataProvider,
        'graphs': graphs,
        'maxZoomFactor': 10000,
        'legend': {
            'divId': 'legend_container',
            'align': 'center',
            'listeners': [{
                'event': 'hideItem',
                'method': function (item) {
                    toggleAllGraphs(item, 'hide');
                }
            }, {
                'event': 'showItem',
                'method': function (item) {
                    toggleAllGraphs(item, 'show');
                }
            }]
        },
        'valueAxes': [{
            'id': 'x',
            'position': 'bottom',
            'dashLength': 1,
            'title': 'Retention time [s]',
            'guides': guides
        }, {
            'id': 'y',
            'dashLength': 1,
            'position': 'left',
            'title': 'Intensity [number of ions]'
        }],
        'chartScrollbar': generatePlotScrollbarDescriptor(),
        'chartCursor': {
            'valueLineAxis': 'y',
            'valueLineBalloonEnabled': true
        },
        'marginLeft': 60,
        'marginBottom': 60,
        'marginRight': 60,
        'export': generatePlotExportDescriptor(graphExporter.xicChartId),
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

document.getElementById('xic_container').addEventListener('click', function (event) {
    xicGraphSelectionState.deselect(event);
});

function createMassPeakChart(div_id, dataProvider, graphs, createSeparateLegend) {
    var result = AmCharts.makeChart(div_id, {
        'type': 'xy',
        'marginLeft': 60,
        'marginBottom': 60,
        'marginRight': 60,
        'autoMarginOffset': 20,
        'startDuration': 0.1,
        'dataProvider': dataProvider,
        'graphs': graphs,
        'maxZoomFactor': 10000,
        'chartScrollbar': generatePlotScrollbarDescriptor(),
        'chartCursor': {
            'valueLineAxis': 'y',
            'valueLineBalloonEnabled': true,
        },
        'legend': {
            'enabled': createSeparateLegend
        },
        'valueAxes': [{
            'id': 'x',
            'position': 'bottom',
            'dashLength': 1,
            'title': 'm/z'
        }, {
            'id': 'y',
            'dashLength': 1,
            'position': 'left',
            'title': 'Intensity [number of ions]'
        }],
        'export': generatePlotExportDescriptor(graphExporter.massPeakChartId),
        'responsive': {
            'enabled': true
        }
    });
    return result;
}

function updateMassChartData(graphDescriptors, points, createSeparateLegend) {
    var massGraphs = getGraphs(graphDescriptors, points, generateMassGraphProto, 0.5, true, massPointAttributeSetter);

    var massPeakChart = chartsById[graphExporter.massPeakChartId];
    if (null !== massPeakChart) {
        massPeakChart.clear();
    }

    chartsById[graphExporter.massPeakChartId] = createMassPeakChart('mass_peak_container', points, massGraphs, createSeparateLegend);
}

function updateChartData(data) {
    xicGraphSelectionState.reset();
    actualPlotData = data;

    var xicGraphs = getGraphs(data[dataController.xicGraphDescKey], data[dataController.xicGraphDataKey], generateXicGraphProto, 0, false, xicPointAttributeSetter);

    var xicChart = chartsById[graphExporter.xicChartId];
    if (null !== xicChart) {
        xicChart.clear();
    }
    chartsById[graphExporter.xicChartId] = createXicChart('xic_container', data[dataController.xicGraphDataKey], xicGraphs, createXicGuides(xicGraphs, data[dataController.xicGraphDescKey]));

    updateMassChartData(data[dataController.ms1GraphDescKey], data[dataController.ms1GraphDataKey], false);
}

dataController.updatePlot.connect(this, updateChartData);
