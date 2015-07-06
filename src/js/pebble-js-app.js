var xhrRequest = function (url, type, data, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText);
	};
	xhr.open(type, url);
	xhr.send(data);
};

function registerEntry() {
	// Send request to MeuPonto
	xhrRequest('http://meu-ponto.appspot.com/register', 'POST', "", function(responseText) {
		// responseText contains a JSON object with weather info
		var json = JSON.parse(responseText);
		console.log(json);

		// Assemble dictionary using our keys
		var dictionary = {
			'STATUS': 0
		};

		// Send to Pebble
		Pebble.sendAppMessage(dictionary, function(e) {
			console.log('Weather info sent to Pebble successfully!');
		}, function(e) {
			console.log('Error sending weather info to Pebble!');
		});
	});

}

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {
	console.log('AppMessage received!');
	registerEntry();
});
