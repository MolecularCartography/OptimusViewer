var chartsById = {};
chartsById[graphExporter.xicChartId] = null;
chartsById[graphExporter.massPeakChartId] = null;

var imageFormats = graphExporter.supportedImageFormatIds;
var dataFormats = graphExporter.supportedDataFormatIds;

var auxPointFlag = 'aux_data';
var graphTitleKey = 'title';
var scanStartKey = 'scan_start';
var graphXFieldKey = 'xField';
var graphYFieldKey = 'yField';
var graphColorKey = 'lineColor';

var actualPlotData = {};

var graphColors = {
    _colorGenerationSize: 10,
    // default amCharts colors
    _colors: ['#FF6600', '#FCD202', '#B0DE09', '#0D8ECF', '#2A0CD0', '#CD0D74', '#CC0000', '#00CC00', '#0000CC', '#DDDDDD', '#999999', '#333333', '#990000'],
    getColor: function(number) {
        if (number >= this._colors.length) {
            this._colors = this._colors.concat(randomColor({count: number - this._colors.length + 1 + this._colorGenerationSize}));
        }
        if (number >= this._colors.length) {
            throw 'Internal error: failed to generate new graph colors';
        }
        return this._colors[number];
    }
};

function clone(obj) {
    if (null == obj || 'object' != typeof obj) return obj;
    var copy = obj.constructor();
    for (var attr in obj) {
        copy[attr] = obj[attr];
    }
    return copy;
}

function getCoordinates(chartId) {
    var chart = chartsById[chartId];
    var graphDescriptorsKey = chartId === graphExporter.xicChartId ? dataController.xicGraphDescKey :
        xicGraphSelectionState._selectionActive ? dataController.msnGraphDescKey : dataController.ms1GraphDescKey;
    var graphDescriptors = actualPlotData[graphDescriptorsKey];

    var resultPoints = [];
    for (var i = 0; i < chart.dataProvider.length; ++i) {
        var originalPoint = chart.dataProvider[i];
        var currentGraph = chart.getGraphById(originalPoint[dataController.graphIdKey]);
        var resultPoint = {};
        if (auxPointFlag in originalPoint || currentGraph.hidden) {
            continue;
        }

        var graphId = originalPoint[dataController.graphIdKey];

        var xKey = graphDescriptors[graphId][dataController.xFieldKey];
        resultPoint['x'] = originalPoint[xKey];

        var yKey = graphDescriptors[graphId][dataController.yFieldKey];
        var graphName = currentGraph[graphTitleKey].replace('<br>', '; ');
        resultPoint['y'] = graphName;
        resultPoint[graphName] = originalPoint[yKey];

        resultPoints.push(resultPoint);
    }
    return resultPoints;
}

function generateFormatListMenu(chartId, formats) {
    var menuItems = [];
    for (var i = 0; i < formats.length; ++i) {
        var formatId = formats[i];
        var menuItem = {
            'label': formatId,
            'click': function (event, menuItem) {
                var chart = chartsById[chartId];

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

                graphExporter.exportGraph(chartId, menuItem.label, getCoordinates(chartId));

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
    if (dataController.precursorMzKey in graphDataItem.dataContext) {
        return 'Scan start time: ' + graphDataItem.values.x.round(2) + ' s<br>(Click to see fragmentation spectrum)<br>'
            + '<b><span style="font-size:14px;">Precursor m/z: ' + graphDataItem.dataContext[dataController.precursorMzKey].round(4) + '</span></b>';
    }
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

function generateMs1GraphTitle(graphDescriptor) {
    return 'Feature ID: ' + graphDescriptor[dataController.featureIdKey]
        + '<br>Consensus m/z: ' + graphDescriptor[dataController.consensusMzGraphKey].round(4)
        + '<br>Sample: ' + graphDescriptor[dataController.sampleNameGraphKey];
}

function generateMs2GraphTitle(graphDescriptor) {
    return 'Precursor m/z: ' + graphDescriptor[dataController.precursorMzKey]
        + '<br>Sample: ' + graphDescriptor[dataController.sampleNameGraphKey]
        + '<br>Scan start time: ' + graphDescriptor[scanStartKey] + ' s';
}

function getGraphs(graphDescriptors, points, graphProtoGenerator, horizontalOffset, stickPlot, pointAttributeSetter, graphTitleGenarator) {
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
        var addAuxAfter = stickPlot || i === points.length - 1 || points[i + 1][dataController.graphIdKey] !== curGraphId;

        var addOffsetBefore = horizontalOffset > 0 && curGraphId !== lastGraphId;
        var addOffsetAfter = horizontalOffset > 0 && i === points.length - 1;

        if (curGraphId !== lastGraphId) {
            var currentGraph = graphProtoGenerator();
            currentGraph[graphXFieldKey] = curGraphDescriptor[dataController.xFieldKey];
            currentGraph[graphYFieldKey] = curGraphDescriptor[dataController.yFieldKey];
            currentGraph[graphTitleKey] = graphTitleGenarator(curGraphDescriptor);
            currentGraph[graphColorKey] = graphColorKey in curGraphDescriptor ? curGraphDescriptor[graphColorKey] : graphColors.getColor(graphs.length);
            currentGraph['id'] = curGraphId;
            graphs.push(currentGraph);
            lastGraphId = curGraphId;
        }

        var xField = curGraphDescriptor[dataController.xFieldKey];
        var yField = curGraphDescriptor[dataController.yFieldKey];
        if (addOffsetBefore) {
            points.splice(i++, 0, createAuxGroundPoint(curPoint, yField, xField, -horizontalOffset));
        }
        if (addAuxBefore && (i === 0 || !(isAuxPoint(points[i - 1]) && points[i - 1][xField] === curPoint[xField]))) { // check if auxilary point is already there
            points.splice(i++, 0, createAuxGroundPoint(curPoint, yField, xField, 0));
        }
        if (addAuxAfter && (i === points.length - 1 || !(isAuxPoint(points[i + 1]) && points[i + 1][xField] === curPoint[xField]))) { // check if auxilary point is already there
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
    _lastUsedColorIndex: -1,

    reset: function() {
        this._selectedItems = [];
        this._selectedCharts = [];
        this._alphasByGraph = [];
        this._lastUsedColorIndex = -1;
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
            this._lastUsedColorIndex = -1;

            delete actualPlotData[dataController.msnGraphDescKey];
            delete actualPlotData[dataController.msnGraphDataKey];

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
                if (this._selectedItems[i]['item'].values.x == event.item.values.x
                    && this._selectedItems[i]['item'].values.y == event.item.values.y)
                {
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
        // color of selected ms2 scan point is the same as color of ms2 spectrum graph
        event.item.dataContext[event.graph.colorField] = graphColors.getColor(++this._lastUsedColorIndex);
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
            var xicPointColor = '';
            for (var i = 0; i < this._selectedItems.length; ++i) {
                if (this._selectedItems[i].item.dataContext[dataController.spectrumIdKey] == graphId) {
                    xicPoint = this._selectedItems[i].item.dataContext;
                    xicPointColor = xicPoint[this._selectedItems[i]['colorField']];
                    break;
                }
            }
            if (null === xicPoint) {
                throw 'Failed to find XIC point with fragmentation spectrum ID ' + graphId;
            }

            var xicGraphDescriptor = actualPlotData[dataController.xicGraphDescKey][xicPoint[dataController.graphIdKey]];

            graphDescriptors[graphId][dataController.precursorMzKey] = xicPoint[dataController.precursorMzKey].round(4);
            graphDescriptors[graphId][scanStartKey] = xicPoint[xicGraphDescriptor[dataController.xFieldKey]].round(2);
            graphDescriptors[graphId][dataController.sampleNameGraphKey] = xicGraphDescriptor[dataController.sampleNameGraphKey];
            graphDescriptors[graphId][graphColorKey] = xicPointColor;
        }
        actualPlotData[dataController.msnGraphDescKey] = graphDescriptors;
        actualPlotData[dataController.msnGraphDataKey] = graphData[dataController.msnGraphDataKey];
        updateMassChartData(graphDescriptors, graphData[dataController.msnGraphDataKey], true);
    }
};

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
            fillColor: graphColors.getColor(i),
            lineColor: graphColors.getColor(i)
        });
    }
    return guides;
}

function getCountOfVisibleGraphs(chart) {
    var visibleCount = 0;
    for (var i = 0; i < chart.graphs.length; ++i) {
        if (!chart.graphs[i].hidden) {
            visibleCount++;
        }
    }
    return visibleCount;
}

function handleXicLegendClick(graph) {
    var chart = graph.chart;
    var actionHide = !graph.hidden;
    if (actionHide && getCountOfVisibleGraphs(chart) < 2) {
        return false;
    }

    var graphIndex = chart.graphs.indexOf(graph);
    if (-1 === graphIndex) {
        throw 'Internal error: couldn\'t find graph in chart';
    }

    for (var chartId in chartsById) {
        var curChart = chartsById[chartId];
        if (curChart == chart) {
            var chartGuides = curChart['valueAxes'][0]['guides'];
            if (actionHide) {
                curChart.hideGraph(graph);
                chartGuides[graphIndex].lineAlpha = 0;
                chartGuides[graphIndex].fillAlpha = 0;
            } else {
                curChart.showGraph(graph);
                chartGuides[graphIndex].lineAlpha = 1;
                chartGuides[graphIndex].fillAlpha = 0.1;
            }
            curChart.validateData();
        } else if (!xicGraphSelectionState._selectionActive) {
            if (actionHide) {
                curChart.hideGraph(curChart.graphs[graphIndex]);
            } else {
                curChart.showGraph(curChart.graphs[graphIndex]);
            }
        }
    }
    return false;
}

function formatNumberAsExponential(value) {
    return value.toExponential(3);
}

function createXicChart(div_id, dataProvider, graphs, guides) {
    var result = AmCharts.makeChart(div_id, {
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
            'switchable': graphs.length > 1,
            'clickMarker': handleXicLegendClick,
            'clickLabel': handleXicLegendClick
        },
        'titles': [{
            'text': 'Extracted Ion Chromatogram of Selected Features'
        }],
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
            'title': 'Intensity [number of ions]',
            'labelFunction': formatNumberAsExponential
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
    return result;
}

function isMouseOverZoomButton(event) {
    var zoomOutButton = this.querySelector('.amcharts-zoom-out-bg');
    if (zoomOutButton) {
        var rect = zoomOutButton.getBoundingClientRect();
        var x = event.clientX;
        var y = event.clientY;
        return !(x < rect.left || x >= rect.left + rect.width || y < rect.top || y >= rect.top + rect.height);
    } else {
        return false;
    }
}

document.getElementById('xic_container').addEventListener('click', function (event) {
    if (!isMouseOverZoomButton.bind(this)(event)) {
        xicGraphSelectionState.deselect(event);
    }
});

function handleMassPeakLegendClick(graph) {
    var chart = graph.chart;
    var actionHide = !graph.hidden;
    if (actionHide && getCountOfVisibleGraphs(chart) < 2) {
        return false;
    }
    if (actionHide) {
        chart.hideGraph(graph);
    } else {
        chart.showGraph(graph);
    }
    return false;
}

function getMassPeakChartTitle(fragmentationSpectra) {
    var title = fragmentationSpectra ? 'Fragmentation Spectra of Selected Scans' : 'Mass Peaks of Selected Features';
    return [{'text': title}];
}

function createMassPeakChart(div_id, dataProvider, graphs, fragmentationSpectra) {
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
            'enabled': fragmentationSpectra,
            'switchable': graphs.length > 1,
            'clickMarker': handleMassPeakLegendClick,
            'clickLabel': handleMassPeakLegendClick
        },
        'titles': getMassPeakChartTitle(fragmentationSpectra),
        'valueAxes': [{
            'id': 'x',
            'position': 'bottom',
            'dashLength': 1,
            'title': 'm/z'
        }, {
            'id': 'y',
            'dashLength': 1,
            'position': 'left',
            'title': 'Intensity [number of ions]',
            'labelFunction': formatNumberAsExponential
        }],
        'export': generatePlotExportDescriptor(graphExporter.massPeakChartId),
        'responsive': {
            'enabled': true
        }
    });
    if (!fragmentationSpectra) { // sync hidden graphs
        var xicGraphs = chartsById[graphExporter.xicChartId].graphs;
        var massPeakGraphs = result.graphs;
        if (xicGraphSelectionState._selectionActive) {
            return;
        }
        for (var i = 0; i < xicGraphs.length; ++i) {
            if (xicGraphs[i].hidden) {
                result.hideGraph(massPeakGraphs[i]);
            }
        }
    }
    return result;
}

function updateMassChartData(graphDescriptors, points, fragmentationSpectra) {
    var massGraphs = getGraphs(graphDescriptors, points, generateMassGraphProto, 0.1, true,
        massPointAttributeSetter, fragmentationSpectra ? generateMs2GraphTitle : generateMs1GraphTitle);

    var massPeakChart = chartsById[graphExporter.massPeakChartId];
    if (null !== massPeakChart) {
        massPeakChart.clear();
    }

    chartsById[graphExporter.massPeakChartId] = createMassPeakChart('mass_peak_container', points, massGraphs, fragmentationSpectra);
}

function updateChartData(data) {
    xicGraphSelectionState.reset();
    actualPlotData = data;

    var xicGraphs = getGraphs(data[dataController.xicGraphDescKey], data[dataController.xicGraphDataKey],
        generateXicGraphProto, 0, false, xicPointAttributeSetter, generateMs1GraphTitle);

    var xicChart = chartsById[graphExporter.xicChartId];
    if (null !== xicChart) {
        xicChart.clear();
    }
    chartsById[graphExporter.xicChartId] = createXicChart('xic_container', data[dataController.xicGraphDataKey],
        xicGraphs, createXicGuides(xicGraphs, data[dataController.xicGraphDescKey]));

    updateMassChartData(data[dataController.ms1GraphDescKey], data[dataController.ms1GraphDataKey], false);
}

dataController.updatePlot.connect(this, updateChartData);
