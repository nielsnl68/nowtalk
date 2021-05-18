
/*jshint esversion: 10 */
"use strict";

const http = require("http");
const fsx = require('fs');
const { parse } = require('querystring');
const process = require('process');

const MAX_MSGS = 100;

class NowTalkHttps {
    constructor(main) {
        // super();
        this.main = main;
        this.config = main.config;
        this.nodes = [];
        this.msgs = [];
        this.clients = [];
        this.timer = false;
        this.onUpdateBadges = this.onUpdateBadges.bind(this);
        this.requestListener = this.requestListener.bind(this);
        this.server = http.createServer(this.requestListener);
        //   this.clientJs = fs.readFileSync("./webpages/dashboard.js")

        if (main.config.webAddress === "*") {
            this.server.listen(main.config.webPort);
        } else {
            this.server.listen(main.config.webPort, main.config.webAddress);
        }
    }

    stop() {
        this.clients.forEach(connection => {
            if (!connection) return;
            connection.end();
        });
        this.server.close();
    }

    requestListener(req, res) {
        //     res.setHeader('Access-Control-Allow-Origin', this.config.sitebaseURL);
        res.setHeader('Access-Control-Request-Method', 'POST,GET');
        res.setHeader('Access-Control-Allow-Methods', 'OPTIONS,POST,GET');
        res.setHeader('Access-Control-Allow-Headers', '*');
        if (req.method === 'OPTIONS') return res.end();
        if (req.method === 'POST' && req.url === '/p') {

            var body = "", http = this;
            req.on("data", function (chunk) {
                body += chunk;
            });

            req.on("end", function () {
                http.postRequest(req, res, parse(body));
                res.end('ok');
            });
        }
        /*        
                if (req.method === 'GET' && req.url === '/client.js') {
                    res.setHeader('Content-type', 'text/javascript');
                    return res.end(this.client_js);
                }
        */
        if (req.headers.accept && req.headers.accept.indexOf('text/event-stream') >= 0) {
            this.handleSSE(res);
            return this.populateNewClient(res);
        }
        switch (req.url) {
            case "":
            case "/":
                res.setHeader("Content-Type", "text/html");
                res.writeHead(200);
                this.indexFile = fsx.readFileSync("./webpages/dashboard.html");
                res.end(this.indexFile);
                break;
            default:
                res.writeHead(404);
                res.end(JSON.stringify({ error: "Resource not found" }));
        }
    }

    handleSSE(res) {
        res.localRef = new Date().toISOString();
        this.clients.push(res);
        res.on('close', () => {
            this.clients.splice(this.clients.findIndex(c => res === c), 1);
            if ((this.clients.length == 0) && this.timer !== false) {
                clearInterval(this.timer);
                this.timer = false;
            }
        });
        res.writeHead(200, {
            'Content-Type': 'text/event-stream',
            'Cache-Control': 'no-cache',
            Connection: 'keep-alive'
        });
        if (this.timer === false) {
            this.timer = setInterval(this.onUpdateBadges, 2500);
        }

    }

    sendSSE(data, connections = false) {
        if (connections === false) {
            connections = this.clients;
        }
        connections.forEach(connection => {
            if (!connection) return;
            const id = new Date().toISOString();
            connection.write('id: ' + id + '\n')
            connection.write('data: ' + data + '\n\n')
        });
        return connections.length !== 0;
    }

    updateConfig(connections = false) {
        return this.sendSSE(JSON.stringify(['config', this.config]), connections);
    }

    addNewBadge(data) {
        return this.sendSSE(JSON.stringify(['newbadge', data]), false);
    }

    addMessage(kind, msg) {
        if (this.msgs.length > MAX_MSGS) {
            this.msgs.shift();
        }
        this.msgs.push([kind, msg]);
        return this.sendSSE(JSON.stringify(['msg', kind, msg]), false);
    }

    populateNewClient(res) {
        this.updateConfig([res]);
        this.sendSSE(JSON.stringify(['msgs', this.msgs]), false);
        this.onUpdateBadges();
    }

    postRequest(req, res, body) {
        if (typeof body.action != "undefined") {
            this.main.emit("web_" + body.action, body);
        }
    }

    onUpdateBadges() {
        let list = [];

        for (const [key, value] of Object.entries(this.main.users)) {
            list.push(value.info(true));
        }
        return this.sendSSE(JSON.stringify(['update', list, process.hrtime.bigint().toString()]), false);
    }

}


module.exports = NowTalkHttps;