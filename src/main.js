Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.openURL('http://mamota.net/pebble/3angles-config.html');
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log('Configuration window returned: ', JSON.stringify(configuration));
    Pebble.sendAppMessage({
      "COLOR_KEY":configuration.color,
      "BG_KEY":configuration.bg
    });
  }
);