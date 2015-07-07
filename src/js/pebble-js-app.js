var xhrRequest = function (url, type, data, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function (e) {
		callback(this.responseText, e.target.status);
	};
	xhr.open(type, url);
	xhr.send(data);
};

serialize = function(obj) {
	var mapping = {
		"YEAR": "year",
		"MONTH": "month",
		"DAY": "day",
		"ENTRY_TYPE": "entry",
		"TIME": "value"
	};
	var str = [];
	for(var param in mapping) {
		if (obj.hasOwnProperty(param)) {
			str.push(mapping[param] + "=" + encodeURIComponent(obj[param]));
		}
	}

	// Hardcoded credentials
	str.push("user=<user>");
	str.push("token=<token>");

	return str.join("&");
}

function registerEntry(e) {

	var data = serialize(e.payload);

	// Send request to MeuPonto
	xhrRequest('http://meu-ponto.appspot.com/register', 'POST', data, function(responseText, statusCode) {
		// responseText contains a JSON object with weather info
		//var json = JSON.parse(responseText);
		console.log(responseText);

		// Assemble dictionary using our keys
		var dictionary = {
			'STATUS': (statusCode == 200) ? 1 : 0
		};

		// Send to Pebble
		Pebble.sendAppMessage(dictionary, function(e) {
			console.log('Result info sent to Pebble successfully!');
		}, function(e) {
			console.log('Error sending result info to Pebble!');
		});
	});

}

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {
	console.log('AppMessage received!');
	registerEntry(e);
});
