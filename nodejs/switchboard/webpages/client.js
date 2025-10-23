
/*jshint esversion: 10 */
$(document).ready(function () {
    'use strict';

    const socket = io();
    const serialObj = navigator.serial;
    //const portObj;// = serialObj.requestPort();

    let table = $("#listBadges");
    let msgBody = $("#msgBody");
    let config;

    let onFindSerialPorts;

    $("#newBadgeAlert").hide();





    // client-side
    socket.on("connect", () => {
        callAddMessage("_connect", "Connected to switchboard");
    });

    socket.on("disconnect", (reason) => {
        callAddMessage("_disconnect", "Server has been disconnected. " + reason);
    });

    socket.on("connect_error", (reason) => {
        //        callAddMessage("_error", "A connect_error. ");
        //      console.error(reason);
    });

    // Get room and users
    socket.on('config', (obj) => {
        $("#sts_switchboardName").text(obj.switchboardName);
        $("#sts_version").text(obj.version + "/" + (obj.bridge.version || "<None>"));
        $("#sts_externIP").text(obj.externelIP || "<None>");
        $("#sts_mac").text(obj.bridge.mac);
        $("#sts_channel").text(obj.bridge.channel);
        $("#sts_bridgeid").text(obj.bridge.bridgeID);
        config = obj;
    });

    socket.on('newBadge', (obj) => {
        if (obj === false) {
            $("#newBadgeAlert").hide();
            $("#newBadgeConfirm").hide();
            $("#newbadgeYes").off('click');
            $("#newbadgeClose").off('click');
            $("#newBadgeForm").off('submit');
            return;
        }
        $("#newBadgeAlert").addClass('show');
        $("#newBadgeConfirm").show();
        $("#newBadgeForm").hide();
        $("#newBadgeAlert").show();

        $("#newbadgeClose").on("click", function () {
            $("#newbadgeClose").off("click");
            $("newBadgeForm").off('submit');
            $("#newbadgeYes").off('click');
            $("#badgeid").val("");
            $("#username").val("");

            $("#newBadgeAlert").hide();
            socket.emit('newBadge', false);
        });

        $("#newbadgeYes").on('click', function () {
            socket.emit('newBadge', true);
            $("#newBadgeConfirm").hide();
            $("#newBadgeForm").show();
            $("#badgeid").val("");
            $("#username").val("");
            $("#newBadgeForm").on("submit", function (event) {
                let form = document.getElementById("newBadgeForm");
                event.preventDefault();
                event.stopPropagation();
                if (!form.checkValidity()) {
                    return;
                }
                form.classList.add('was-validated');
                socket.emit('newBadge',
                    {
                        badgeid: $("#badgeid").val(),
                        username: $("#username").val(),
                    });

                $("#newbadgeClose").trigger("click");
            });
        });
    });

    socket.on('msg', (kind, msg) => callAddMessage(kind, msg));

    socket.on('msgs', (list) => {
        $("#msgbody").empty();
        list.forEach((item) => {
            callAddMessage(item[0], item[1]);
        });
    });

    socket.on('update', (list, time) => {
        list.forEach((item) => {
            //       return { mac: useHexMac ? this._mac : this.mac, status: this.status, ip: this.ip, name: this.name, time: this.timestamp };
            let status_icon = 'handshake'; // item.status.toString(16);
            let status_color = 'secondary';
            let status_hint = 'Status code: ' + item.status.toString(16);
            let isActive = ((item.status & 0x01) == 0x01);
            if ((item.status) == 0x00) {
                status_color = "primary";
                status_hint = "Handshake";
            } else if ((item.status & 0x70) == 0x10) {
                status_icon = "user-alt";
                if (isActive) status_color = 'success';
                status_hint = "Local User";
            } else if ((item.status & 0x78) == 0x18) {
                status_icon = "people-arrows";
                if (isActive) status_color = 'success';
                status_hint = "Outside user";
            } else if ((item.status & 0x70) == 0x20) {
                status_icon = "users";
                if (isActive) status_color = 'success';
                status_hint = "Family or friends";
            } else if ((item.status & 0x72) == 0x02 || (item.status & 0xFF) == 0x80) {
                status_icon = "eye";
                if (isActive) status_color = 'success';
                status_hint = "Guest";
            } else {
                status_icon = "exclamation-circle";
                status_color = "info";
            }
            if ((item.status & 0x80) == 0x80) {
                status_icon += "-slash";
                status_color = 'danger';
                status_hint += ' disabled';
            }
            let statusClass = 'nowtalk-status text-' + status_color + ' me-1 fas fa-' + status_icon;

            if (table.has("#" + item.mac).length == 0) {
                let newRow = "<tr id='" + item.mac + "' class='align-middle'>";
                newRow += "<td class='text-center'>";
                newRow += '<i data-action="status"  class="' + statusClass + '" title="' + status_hint + '" data-bs-toggle="dropdown" aria-expanded="false" role="button" tabindex="0"></i>';
                newRow += "<ul class='dropdown-menu dropdown-menu-dark ' style='min-width:none'><li>";

                newRow += '<div class="p-0 btn-group">';
                newRow += '<button class="dropdown-item nowtalk-toggle" data-action="friend" title="Make a friend"><i class="fas fa-users" ></i></button> ';
                newRow += '<button class="dropdown-item nowtalk-edit" data-action="name" title="Edit username"><i class="fas fa-edit" ></i></button> ';
                newRow += '<button class="dropdown-item text-danger nowtalk-toggle" data-action="disable" title="Block badge"><i class="fas fa-times" ></i></button> ';
                newRow += "</div></li></ul";

                newRow += "</td > ";
                newRow += "<td class='text-truncate'>" + item.mac.substr(1) + "</td>";
                newRow += "<td class='nowtalk-name text-truncate'></td>";
                newRow += "<td class='nowtalk-ip text-truncate' ></td><td class=''>";
                newRow += "<div class='progress my-2'>";
                newRow += '<div class="progress-bar" role="progressbar" aria-valuenow="50" aria-valuemin="0" aria-valuemax="100" style="width: 50%"></div>';
                newRow += "</div></td></tr >";
                table.append(newRow);
            }
            let row = $("#" + item.mac);
            row.attr('data-status', "H" + ("0" + item.status.toString(16)).substr(-2));

            row.attr('data-progress', item.time);
            let statusDOM = row.find('[data-action="status"]');
            let oldclass = statusDOM.attr('class');
            if (statusDOM.hasClass('show')) { statusClass += " show"; }
            if (oldclass !== statusClass) {
                //      console.info([statusClass, oldclass]);
                row.find('[data-action="status"]').attr('class', statusClass);
                row.find('[data-action="status"]').attr('title', status_hint);
            }
            row.children(".nowtalk-name").text(item.name);
            row.children(".nowtalk-ip").text(item.ip);
            let x = item.time;
            row.find(".progress-bar").attr("aria-valuenow", x).css("width", x + "%");

            row.find('[data-action="friend"]').prop('disabled', (item.status & 0x10) !== 0).attr('data-status', (item.status & 0x20) !== 0);
            row.find("[data-action='name']").prop('disabled', (item.status & 0x30) === 0).attr('data-name', item.name);
            row.find('[data-action="disable"]').attr('data-status', (item.status & 0x80) !== 0);
            row.attr('data-status', ("0" + item.status.toString(16)).substr(-2));
            if (typeof time === "string") {
                row.attr('data-time', time);
            }
        });

        if (typeof time === "string") {
            table.children('[data-time!=' + time + ']').remove();
        }
    });


    function callAddMessage(kind, msg) {
        let row = $("<div class='row'></div>");
        let _msg = $("<div class='m-0  p-0 small'  role='alert'>" + msg + "</div>");
        switch (kind) {
            case 'recv':
                _msg.addClass("col-10 px-1 mt-1 text-success border rounded-end alert-secondary  ");
                break;
            case 'send':
                row.append("<div class='col-2'></div>");
                _msg.addClass("text-end col-10 mt-1 px-1 text-primary rounded-start alert-secondary ");
                break;

            case '_connect':
                _msg.addClass("col-12 px-1 mt-1 text-success border alert-success  ");
                break;
            case '_disconnect':
                _msg.addClass("col-12 px-1 mt-1 text-danger border alert-danger  ");
                break;
            case '_error':
                _msg.addClass("col-12 px-1 mt-1 text-warning border alert-warning  ");
                break;
            default:
                _msg.addClass("col-12 text-" + kind);
        }
        row.append(_msg);
        msgBody.append(row);
        var elem = document.getElementById('msgBody');
        elem.scrollTop = elem.scrollHeight;
    }

    function checkBadgeID(code) {
        let baseChars = "0123456789AbCdEfGhIjKlMnOpQrStUvWxYz";
        const baseCount = baseChars.length + 1;
        baseChars += "aBcDeFgHiJkLmNoPqRsTuVwXyZ";
        let length = code.length;
        var count = 0;
        var arr = [];
        if (length != 8) return false;
        for (let index = 6; index >= 0; index--) {
            let chr = code.charAt(index);
            count += baseChars.indexOf(chr);
            arr.push([count, baseChars.indexOf(chr), chr]);
        }
        let crc = count % baseCount, chr = code.charAt(7);
        return crc === baseChars.indexOf(chr);
    }

    $("#badgeid").on("input", function (event) {
        let id = $(this).val();
        if (!checkBadgeID(id)) {
            email.setCustomValidity("This is an invalid badgeID. The code is case sensitive!");
        } else {
            email.setCustomValidity("");
        }
    });

    table.on('click', ".nowtalk-toggle", (e) => {
        let button = $(this.activeElement);
        let row = button.closest('tr');
        socket.emit('editBadge', row.prop('id'), button.data('action'), button.data("status"));
    });

    table.on('click', '.nowtalk-edit', (e) => {
        let button = $(this.activeElement);
        let row = button.closest('tr');

        var person = prompt("Please change the username", button.data('name'));

        if (person != null) {
            socket.emit('editBadge', row.prop('id'), button.data('action'), person);
        }
    });

    table.on('contextmenu', '.nowtalk-status', (e) => {
        e.preventDefault();
        //     console.info('contextmenu clicked');
        $(this).trigger('click');
        return false;
    });

    function showlog(log) {
        callAddMessage("primary", log);
    }

    let espPort;

    async function nowTalkSend(packet) {
        if (typeof packet === "string") {
            packet = new TextEncoder("utf-8").encode(packet);
        }
        const espWriter = espPort.writable.getWriter();
        console.log("Send", packet);
        await espWriter.write(packet);
        espWriter.releaseLock();

        // Wait
 //       await new Promise(resolve => setTimeout(resolve, 100));
    }


    async function nowTalkTimeout(espReader) {

        if (espReader) {
            console.info("timeout", espReader);
            showlog("timeout" );
            espReader.cancel("cancel at Timeout");
        }
    }

    async function nowTalkReceive(timeOut = 200) {
        let espReader = espPort.readable.getReader();
        let timer = setTimeout(nowTalkTimeout, timeOut, espReader);
        let result = "";
        console.warn(espReader);
        showlog("start reading");
        try {
            while (true) {
                const { value, done } = await espReader.read();
                if (done) {
                    showlog("done");
                    break;
                }
                result += new TextDecoder().decode(value);
            }
        } finally {
            clearTimeout(timer);
            espReader.cancel("cancel at done");
            await espReader.releaseLock();
            espReader = null;
        }
        return result;
    }


    $("#btnConnect").on('click', writeBtn);
    async function writeBtn() {
        espSetOutput(showlog);
        try {
            espPort = await espSelectPort(115200);
            await nowTalkReceive();
            nowTalkSend("###info~" + config.bridge.mac + "\n");
            let value = await nowTalkReceive();
            showlog(value);
            if (value) {
                value = value.split("~");
                showlog(value);
            }

        } catch (error) {
            showlog(error);
        } finally {
            await espDisconnect(true);
        }

    };
});