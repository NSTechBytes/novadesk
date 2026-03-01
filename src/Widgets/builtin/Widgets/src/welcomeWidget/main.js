var utils = require('../common/utils');

var welcome_Widget = null;

function loadWelcomeWidget() {
    if (welcome_Widget) {
        return; // Widget already registered
    }

    welcome_Widget = new widgetWindow({
        id: 'welcome_Window',
        script: 'ui/widget.ui.js',
        width: 300,
        height: 300,
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {
        welcome_Widget.setProperties({
            x: ((system.getDisplayMetrics().primary.screenArea.width - 300) / 2),
            y: ((system.getDisplayMetrics().primary.screenArea.height - 300) / 2),
            show: true
        });
    }

    // Set up context menu
    welcome_Widget.setContextMenu([
        {
            text: "Refresh",
            action: function () {
                welcome_Widget.refresh();
            }
        },
        { type: "separator" },
        {
            text: "Close",
            action: function () {
                utils.setJsonValue('welcome_Widget_Active', false);
                unloadWelcomeWidget();
                if (typeof __refreshTrayMenu === "function") {
                    __refreshTrayMenu();
                }
            }
        }
    ]);
}

// ipcMain listeners for button clicks
ipcMain.on("openWebsite", function () {
    system.execute("https://novadesk.pages.dev/");
});

ipcMain.on("openDocs", function () {
    system.execute("https://novadesk-docs.pages.dev/");
});

function unloadWelcomeWidget() {
    if (welcome_Widget) {
        welcome_Widget.close();
        welcome_Widget = null;
    }
}

module.exports = {
    loadWelcomeWidget,
    unloadWelcomeWidget
}

