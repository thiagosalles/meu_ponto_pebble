var xhrRequest = function (url, type, data, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText, this.status);
	};
	xhr.open(type, url, true);
	if (type == "POST") {
		xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
		xhr.setRequestHeader("Content-length", data.length);
		xhr.setRequestHeader("Connection", "close");
		xhr.send(data);
	} else {
		xhr.send();
	}
};

hasCredentials = function() {
	return !!(localStorage.getItem('user') && localStorage.getItem('token'));
}

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

	var user = localStorage.getItem('user') || "";
	var token = localStorage.getItem('token') || "";

	// Hardcoded credentials
	str.push("user=" + encodeURIComponent(user));
	str.push("token=" + encodeURIComponent(token));

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

Pebble.addEventListener('showConfiguration', function(e) {
	var params = serialize({});
	Pebble.openURL('http://meu-ponto.appspot.com/pebble?' + params);
});

Pebble.addEventListener('webviewclosed', function(e) {
	var configuration = JSON.parse(decodeURIComponent(e.response));
	if (configuration.clear) {
		localStorage.clear();
	} else if (configuration.user && configuration.token) {
		localStorage.setItem('user', configuration.user.trim());
		localStorage.setItem('token', configuration.token.trim());
	}
	console.log('Configuration window returned: ', JSON.stringify(configuration));
});

Pebble.addEventListener('ready', function(e) {
	console.log('Has credentials? ' + hasCredentials());
});
