// Test for ElementLayoutBox display property

exports.main = async function (novadesk) {
    const widget = await novadesk.createWidget({
        id: 'displayTest',
        x: 100,
        y: 100,
        width: 600,
        height: 400,
        backgroundColor: 'rgba(30, 30, 30, 255)'
    });

    // Test various display types
    widget.ui.addLayoutBox({
        id: 'blockBox',
        x: 10,
        y: 10,
        width: 280,
        height: 80,
        display: 'block',
        backgroundColor: 'rgba(60, 120, 180, 200)',
        borderWidth: 2,
        borderColor: 'rgba(100, 160, 220, 255)',
        borderRadius: 5
    });

    widget.ui.addLayoutBox({
        id: 'flexBox',
        x: 10,
        y: 100,
        width: 280,
        height: 80,
        display: 'flex',
        direction: 'ltr',
        backgroundColor: 'rgba(180, 60, 120, 200)',
        borderWidth: 2,
        borderColor: 'rgba(220, 100, 160, 255)',
        borderRadius: 5
    });

    widget.ui.addLayoutBox({
        id: 'inlineBlockBox',
        x: 10,
        y: 190,
        width: 280,
        height: 80,
        display: 'inlineBlock',
        backgroundColor: 'rgba(120, 180, 60, 200)',
        borderWidth: 2,
        borderColor: 'rgba(160, 220, 100, 255)',
        borderRadius: 5
    });

    widget.ui.addLayoutBox({
        id: 'gridBox',
        x: 310,
        y: 10,
        width: 280,
        height: 80,
        display: 'grid',
        backgroundColor: 'rgba(180, 120, 60, 200)',
        borderWidth: 2,
        borderColor: 'rgba(220, 160, 100, 255)',
        borderRadius: 5
    });

    widget.ui.addLayoutBox({
        id: 'noneBox',
        x: 310,
        y: 100,
        width: 280,
        height: 80,
        display: 'none',
        backgroundColor: 'rgba(60, 180, 120, 200)',
        borderWidth: 2,
        borderColor: 'rgba(100, 220, 160, 255)',
        borderRadius: 5
    });

    // Test with style object
    widget.ui.addLayoutBox({
        id: 'inlineFlexBox',
        x: 310,
        y: 190,
        width: 280,
        height: 80,
        style: {
            display: 'inlineFlex'
        },
        backgroundColor: 'rgba(120, 60, 180, 200)',
        borderWidth: 2,
        borderColor: 'rgba(160, 100, 220, 255)',
        borderRadius: 5
    });

    // Add text labels
    widget.ui.addText({
        id: 'blockLabel',
        x: 20,
        y: 35,
        text: 'display: block',
        fontColor: 'rgba(255, 255, 255, 255)',
        fontSize: 14
    });

    widget.ui.addText({
        id: 'flexLabel',
        x: 20,
        y: 125,
        text: 'display: flex',
        fontColor: 'rgba(255, 255, 255, 255)',
        fontSize: 14
    });

    widget.ui.addText({
        id: 'inlineBlockLabel',
        x: 20,
        y: 215,
        text: 'display: inlineBlock',
        fontColor: 'rgba(255, 255, 255, 255)',
        fontSize: 14
    });

    widget.ui.addText({
        id: 'gridLabel',
        x: 320,
        y: 35,
        text: 'display: grid',
        fontColor: 'rgba(255, 255, 255, 255)',
        fontSize: 14
    });

    widget.ui.addText({
        id: 'noneLabel',
        x: 320,
        y: 125,
        text: 'display: none (hidden)',
        fontColor: 'rgba(255, 255, 255, 255)',
        fontSize: 14
    });

    widget.ui.addText({
        id: 'inlineFlexLabel',
        x: 320,
        y: 215,
        text: 'display: inlineFlex',
        fontColor: 'rgba(255, 255, 255, 255)',
        fontSize: 14
    });

    // Test getting display property
    widget.ui.addText({
        id: 'info',
        x: 10,
        y: 280,
        text: 'Display types: inline, block, inlineBlock, flex, contents, inlineFlex, grid, inlineGrid, table, inlineTable, listItem, none, runIn',
        fontColor: 'rgba(200, 200, 200, 255)',
        fontSize: 10,
        width: 580
    });

    widget.show();

    // Log the display property values
    console.log('Block display:', widget.ui.getElementProperty('blockBox', 'display'));
    console.log('Flex display:', widget.ui.getElementProperty('flexBox', 'display'));
    console.log('Grid display:', widget.ui.getElementProperty('gridBox', 'display'));
};
