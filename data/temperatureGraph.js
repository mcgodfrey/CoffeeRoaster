/*
  Coffee roaster Client-code.
  2 interfaces: 
  - simple
  - program

  == Simple interface ==
  - Set PID parameters
  - Set setpoint temperature and target ramp rate
  - Start/Stop the roaster

  Status on the RHS is updated whenever anything changes (tempreature periodically)
  A chart with the setpoint, measured temp and output are displayed when you press start, updated in real time

  == Program interface ==
  Not implemented yet
  Program in a full ramp profile and set the roaster to follow it.
  program is a JSON file with settings and ramp-rates/setoints
  eg:
  {preheat_temp: 120, 
  ramp_rate: 200,
  hold_temp: 320, 
  hold_Time: 120,
  p: 10,
  i: 0,
  d: 0
  }


  == Structure ==
  Data is sent from server to client through a websocket interface
  Data is send/received as JSON

  When the websocket is opened, the server will send the current status in full.
  Whenever anything changes on the server, it will send only the parameters which have changed.
  This client should listen to all messages and update the display as they are received. No need to poll.

  Data is sent one datapoint at a time from the server as it is measured.
  This client should listen for new datapoints and update the display.


  === Status info from the server ===
  {"type": "status",
  "data": ["param": value, ...]}
  eg.
  {"type": "status",
  "data": ["p":10, "i":1, "d":0, "filename":"temp.csv"]}


  === Datapoint from the server ===
  {"type":"data",
  "data":"timestamp,setpoint,output,temperature"}
  eg.
  {"type":"data",
  "data":"123456789,125,75,103"}

*/

var connection;

var states = ["OFF", "PREHEATING", "PREHEAT", "RAMPING", "HOLD", "COOLING"]

var t0 = -1;

var timerPointer;
var myChart;

/*
 * Callback function when a websocket message is received from the server
 */
websocket_message = function (e) {
    console.log('Message received: <', e.data, '>');
    var msg;
    try{
        msg = JSON.parse(e.data);
    } catch(e) {
        msg = {type: "string", data:e.data};
    }
    if(msg.type == "status"){
        var data = msg.data;
        console.log("data = ", data);
        for (var key in data) {
            if (data.hasOwnProperty(key)) {
                if(key == "state"){
                    var state = data[key];
                    document.getElementById("status_state").innerHTML = "State = " + states[state];
                    if(state == 0){
                        document.getElementById("simple_start_stop_button").innerHTML = "Start";
                        document.getElementById("simple_restart_button").style.display = "none";
                    }else{
                        document.getElementById("simple_start_stop_button").innerHTML = "Stop";
                        document.getElementById("simple_restart_button").style.display = "";
                    }
                }else if(key == "temperature"){
                    document.getElementById("status_temp").innerHTML = "Temp = " + data[key];
                }else if(key == "ramp_rate"){
                    document.getElementById("setpoint_ramp_rate").value = data[key];
                }else if (key == "duty_cycle"){
                    document.getElementById("status_duty_cycle").innerHTML = "duty_cycle = " + data[key];
                }else if (key == "output"){
                    document.getElementById("status_duty_cycle").innerHTML = "duty_cycle = " + data[key];
                }else if (key == "setpoint"){
                    document.getElementById("status_setpoint").innerHTML = "setpoint = " + data[key];
                }else if (key == "p"){
                    document.getElementById("status_P").innerHTML = "P = " + data[key];
                    document.getElementById("setpoint_P").value = data[key];
                }else if (key == "i"){
                    document.getElementById("status_I").innerHTML = "I = " + data[key];
                    document.getElementById("setpoint_I").value = data[key];
                }else if (key == "d"){
                    document.getElementById("status_D").innerHTML = "D = " + data[key];
                    document.getElementById("setpoint_D").value = data[key];
                }else if (key == "filename"){
                    filename = data[key];
                }
                console.log(key + " -> " + data[key]);
            }
        }
    }else if(msg.type == "data"){
        var data = msg.data.split(",", 4);
        if(t0 < 0){
            t0 = parseInt(data[0]);
        }
        var timestamp = parseInt(data[0]) - t0;
        myChart.data.datasets[0].data.push({x: timestamp, y: parseFloat(data[3])});
        myChart.data.datasets[1].data.push({x: timestamp, y: parseFloat(data[1])});
        myChart.data.datasets[2].data.push({x: timestamp, y: parseFloat(data[2])});
        //setpoints.push({x: timestamp, y: parseFloat(data[1])});
        //outputs.push({x: timestamp, y: parseFloat(data[2])});
        //temperatures.push({x: timestamp, y: parseFloat(data[3])});
        //drawChart();
        myChart.update();
    }else{
        console.log("Unhandled message type");
    }
}



/*
 * Main init funciton. Opens a websocket connection to the server and then everything is event driven after that
 * It would be nice to reconnect to the server if the socket is dropped for some reason.
 */
function init() {
    myChart = drawChart();
    
    connection = new WebSocket('ws://'+location.host+':81/', ['arduino']);
    connection.onopen = function () {
        console.log('new connection');
    };
    connection.onmessage = websocket_message;

    connection.onerror = function (error) {
        console.log('WebSocket Error ', error);
    };

    connection.onclose = function () {
        console.log('WebSocket connection closed');
    };
}


/*
 * Create a new chart and fill it with data
 * Eventually I want to have a way of updating the chart, instead of redrawing it every time
 */
function drawChart(){
    var ctx = document.getElementById("myChart").getContext('2d');
    var myChart = new Chart(ctx, {
        type: 'line',
	data: {
	    datasets: [{
		label: 'temperature',
		fill: false,
		backgroundColor: 'rgba(255, 0, 0, 0.2)',
		borderColor: 'rgba(255, 0, 0, 0.2)',
		data: [],
	    }, {
		label: 'setpoint',
		fill: false,
		backgroundColor: 'rgba(0, 0, 255, 0.2)',
		borderColor: 'rgba(0, 0, 255, 0.2)',
		data: [],
	    }, {
		label: 'output',
		fill: false,
		backgroundColor: 'rgba(0, 255, 0, 0.2)',
		borderColor: 'rgba(0, 255, 0, 0.2)',
		data: [],
	    }]
	},
	options: {
	    responsive: true,
            animation: false,
	    title: {
		display: false,
		text: 'Chart.js Line Chart'
	    },
	    tooltips: {
		mode: 'index',
		intersect: false,
	    },
	    hover: {
		mode: 'nearest',
		intersect: true
	    },
	    scales: {
		xAxes: [{
                    type: 'linear',
		    display: true,
		    scaleLabel: {
			display: true,
			labelString: 'Time [s]'
		    }
		}],
		yAxes: [{
		    display: true,
		    scaleLabel: {
			display: true,
			labelString: 'Temperature [oC]'
		    }
		}]
	    }
	}
    });
    return myChart;
}


document.getElementById("switch_to_program_mode_button").onclick = function() {
    document.getElementById("program_controls").style.display = "";
    document.getElementById("simple_controls").style.display = "none";
    connection.send('{"commands":["program_mode"]}');

}

document.getElementById("switch_to_simple_mode_button").onclick = function() {
    document.getElementById("simple_controls").style.display = "";
    document.getElementById("program_controls").style.display = "none";
    connection.send('{"commands":["simple_mode"]}');
}


window.addEventListener('resize', function () { myChart.resize() })


/**************************************************
 * Simple interface functions
 **************************************************/

function change_setpoint() {
    console.log("Changing Setpoint");
    var new_setpoint = String(document.getElementById("setpoint_temp").value);
    connection.send('{"parameters":{"setpoint":'+new_setpoint+'}}');
}

function change_ramp_rate() {
    var ramp_rate = String(document.getElementById("setpoint_ramp_rate").value);
    connection.send('{"parameters":{"ramp_rate":'+ramp_rate+'}}');
}

function change_p() {
    var p = String(document.getElementById("setpoint_P").value);
    connection.send('{"parameters":{"p":'+p+'}}');
}

function change_i() {
    var i = String(document.getElementById("setpoint_I").value)
    connection.send('{"parameters":{"i":'+i+'}}');
}

function change_d() {
    var d = String(document.getElementById("setpoint_D").value)
    connection.send('{"parameters":{"d":'+d+'}}');
}

function save_config(){
    connection.send('{"commands":["saveConfig"]}');
}

function start_controller(){
    document.getElementById("simple_start_stop_button").innerHTML = "Stop";
    document.getElementById("simple_restart_button").style.display = "";
    t0 = -1;
    myChart = drawChart();
    
    connection.send('{"commands":["start"]}');
}

function stop_controller(){
    document.getElementById("simple_start_stop_button").innerHTML = "Start";
    document.getElementById("simple_restart_button").style.display = "none";
    t0 = -1;

    connection.send('{"commands":["stop"]}');
}


function restart_controller() {
    // Reset the arrays to empty to start a new run
    t0 = -1;
    myChart = drawChart();

    connection.send('{"commands":["restart"]}');
}


/* assign callbacks */
document.getElementById("setpoint_temp").onchange = change_setpoint;
document.getElementById("setpoint_ramp_rate").onchange = change_ramp_rate;
document.getElementById("setpoint_P").onchange = change_p;
document.getElementById("setpoint_I").onchange = change_i;
document.getElementById("setpoint_D").onchange = change_d;
document.getElementById("save_config_button").onclick = save_config;
document.getElementById("simple_restart_button").onclick = restart_controller;
document.getElementById("simple_start_stop_button").onclick = function() {
    var b = document.getElementById("simple_start_stop_button")
    if(b.innerHTML == "Start"){
        start_controller();
    } else {
        stop_controller();
    }
}

/**************************************************
 * program interface functions
 **************************************************/
/*
 * send the program roast profile to the micro and start preheating
 */
document.getElementById("program_preheat_button").onclick = function() {
    document.getElementById("program_start_button").disabled = false;
    document.getElementById("program_abort_button").disabled = false;
    document.getElementById("program_preheat_button").disabled = true;
    document.getElementById("program_filename").disabled = true;
}

/*
 * Preheat is finished, you have added the beans, start the roast
 */
document.getElementById("program_start_button").onclick = function() {
    document.getElementById("program_start_button").disabled = true;
}

/*
 * Finish the roast eary for some reason.
 */
document.getElementById("program_abort_button").onclick = function() {
    document.getElementById("program_preheat_button").disabled = false;
    document.getElementById("program_filename").disabled = false;
    document.getElementById("program_abort_button").disabled = true;
}



$(document).ready(function(){
    var num_steps = 1;

    $("#add_step").on("click", function(){
        console.log("On click!");
        num_steps += 1;

        var new_row = "<tr>";
        new_row += '<td><input class="form-control" type="text" placeholder="step name"></td>';
        new_row += '<td> <input class="form-control" type="number" placeholder="Target Temperature"></td>';
        new_row += '<td> <input class="form-control" type="number" placeholder="Ramp Rate"></td>';
        new_row += '<td> <input class="form-control" type="number" placeholder="Hold Time"></td>';
        new_row += '<td> <button type="button" class="close"><span>&times;</span></button></td>';
        new_row += "</tr>"
        $("#program_table").find("tbody").append(new_row);
    });


    $("#program_table").on("click", ".close", function(){
        if(num_steps > 1){
            num_steps -= 1;
            var i = this.parentNode.parentNode.rowIndex;
            document.getElementById("program_table").deleteRow(i);
        }else{
            alert("You need at least one row");
        }
    })
    
    $("#upload_program").on("click", function(){
        var program_steps = [];
        var program_name = document.getElementById("program_name").value;
        var program_table = document.getElementById("program_table");
        for(var i=0, row; row = program_table.tBodies[0].rows[i]; i++){
            console.log(row);
            console.log(row.cells[0].children[0].value);
            var step_name = row.cells[0].children[0].value;
            var target_temp = row.cells[1].children[0].value;
            var ramp_rate = row.cells[2].children[0].value;
            var hold_time = row.cells[3].children[0].value;
            program_steps.push({step_name:step_name, target_temp:target_temp, ramp_rate:ramp_rate, hold_time:hold_time});
        }
        var data = {program_name:program_name, program_steps:program_steps};
        var message = {command:"upload_program", data:data};
        message_str = JSON.stringify(message);
        console.log(message_str);
        
    });
});


init();
