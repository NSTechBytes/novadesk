var config_Path = path.join(app.getAppDataPath(), 'config.json');

var config_Data = system.readJson(config_Path);


function pad2(n) {
    return (n < 10 ? "0" : "") + n;
}

function formatTime(dateObj, use12h) {
    var h = dateObj.getHours();
    var m = dateObj.getMinutes();
    var s = dateObj.getSeconds();

    if (use12h) {
        var h12 = h % 12;
        if (h12 === 0) h12 = 12;
        var ampm = h >= 12 ? "PM" : "AM";
        return pad2(h12) + ":" + pad2(m) + ":" + pad2(s) + " " + ampm;
    }

    return pad2(h) + ":" + pad2(m) + ":" + pad2(s);
}

function formatDay(dateObj) {
    var days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
    return days[dateObj.getDay()];
}

function formatDate(dateObj) {
    var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
    var monthName = months[dateObj.getMonth()];
    var date = dateObj.getDate();
    var year = dateObj.getFullYear();
    return monthName + " " + date + ", " + year;
}

function getJsonValue(key) {
    return config_Data[key];
}

function setJsonValue(key, value) {
    config_Data[key] = value;
    system.writeJson(config_Path, config_Data);
}

function kelvinToCelsius(kelvin) {
    return Math.round(kelvin - 273.15);
}

function celsiusToFahrenheit(celsius) {
    return Math.round((celsius * 9/5) + 32);
}

function getWeatherIcon(code) {
    // Map weather codes to simple ASCII icons
    var icons = {
        0: "☀", // Clear sky
        1: "🌤", // Mainly clear
        2: "⛅", // Partly cloudy
        3: "☁", // Overcast
        45: "🌫", // Fog
        51: "🌦", // Light drizzle
        61: "🌧", // Rain
        65: "⛈", // Heavy rain
        71: "❄", // Snow
        95: "⛈"  // Thunderstorm
    };
    return icons[code] || "?";
}

module.exports = {
    formatTime: formatTime,
    formatDay: formatDay,
    formatDate: formatDate,
    getJsonValue: getJsonValue,
    setJsonValue: setJsonValue,
    kelvinToCelsius: kelvinToCelsius,
    celsiusToFahrenheit: celsiusToFahrenheit,
    getWeatherIcon: getWeatherIcon,
    pad2: pad2
};
