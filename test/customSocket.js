const net = require('net');


async function customSocket(host, port, content){
    return new Promise((resolve, reject) => {
        const client = new net.Socket();

        client.connect(port, host, function () {
            client.write(content);
        });

        client.on('data', function (data) {
            client.end();
            resolve(data.toString());
        });

        client.on('error', function (err) {
            client.end()
            reject(err);
        });

        client.on('close', function () {
            reject("closed")
        });
    })
}

module.exports = customSocket;