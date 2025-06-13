const request = require('supertest');
const net = require("net");
const customSocket = require('./customSocket');

const url = 'http://localhost:8080';
describe('test headers', function () {
    it('can connect', function (done) {
        request(url)
            .get('/')
            .expect(200)
            .expect('Hello, World!', done)
    });

    // the client_max_header_count is set to 1 in the server config in the http part
    // so we expect a 400 Bad Request if we send more than one header
    // because the server is with the new client_max_header_count is only after the host header is found
    it('host header not first place', async function () {
        const data = await customSocket("localhost", 8080,
            'GET / HTTP/1.1\r\n' +
            'Connection: close\r\n' +
            'Host: 127.0.0.1:8080\r\n' +
            '\r\n' +
            '\r\n'
        )

        if (!data.includes('HTTP/1.1 400 Bad Request')) {
            throw new Error("Expected 400 Bad Request, got: " + data);
        }
    });

    it("lower as max header count", async function () {
        const req = request(url)
            .get('/')

        for (let i = 0; i < 15; i++) {
            req.set(`test${i}`, `test${i}`);
        }
        await req.expect(200);
    })

    it("too many headers", async function () {
        const req = request(url)
            .get('/')

        for (let i = 0; i < 21; i++) {
            req.set(`test${i}`, `test${i}`);
        }
        await req.expect(400);
    })

    it("too long header", function (done) {
        const longHeader = 'A'.repeat(1126);
        request(url)
            .get('/')
            .set("test", longHeader)
            .expect(400, done)
    })

    it("good header", function (done) {
        const longHeader = 'A'.repeat(500);
        request(url)
            .get('/')
            .set("test", longHeader)
            .expect(200, done)
    })


    it("too large request line", async function () {
        const longRequestLine = 'A'.repeat(103);
        const data = await customSocket("localhost", 8080,
            `GET /${longRequestLine} HTTP/1.1\r` +
            'Connection: close\r\n ' +
            'Host: 127.0.0.1:8080\r\n ' +
            '\r\n ' +
            '\r\n '
        )

        if (!data.includes('414 Request URI Too Long')) {
            throw new Error("Expected 414 Request URI Too Long, got: " + data);
        }
    })

    it("duplicated host header", async function () {
        const content = 'GET / HTTP/1.1\r\n' +
            'Host: localhost\r\n' +
            'Connection: close\r\n' +
            'Host: localhost:8080\r\n' +
            '\r\n' +
            '\r\n'

        const data = await customSocket("localhost", 8080, content)
        if (!data.includes('HTTP/1.1 400 Bad Request')) {
            throw new Error("Expected 400 Bad Request, got: " + data);
        }
    })

    it("wrong formated header", async function () {
        const content = 'GET / HTTP/1.1\r\n' +
            'Host: localhost\r\n' +
            'Connection: close\r\n' +
            'test localhost:8080\r\n' + // missing colon
            '\r\n' +
            '\r\n'

        const data = await customSocket("localhost", 8080, content)
        if (!data.includes('HTTP/1.1 400 Bad Request')) {
            throw new Error("Expected 400 Bad Request, got: " + data);
        }
    })
});



