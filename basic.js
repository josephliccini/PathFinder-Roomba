var EddystoneBeaconScanner = require('eddystone-beacon-scanner');

var beacons = [];

EddystoneBeaconScanner.on('found', function(beacon) {
	beacons.push(beacon);
});

EddystoneBeaconScanner.on('updated', function(beacon) {
// console.log('updated Eddystone Beacon:\n', JSON.stringify(beacon, null, 2));
});

EddystoneBeaconScanner.on('lost', function(beacon) {
//  console.log('lost Eddystone beacon:\n', JSON.stringify(beacon, null, 2));
});

EddystoneBeaconScanner.startScanning(false);

setTimeout(function() {
	console.log(JSON.stringify(beacons));

	EddystoneBeaconScanner.stopScanning();

	process.exit(0);
}, 8000);
