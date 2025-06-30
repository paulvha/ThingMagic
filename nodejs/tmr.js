/*
 * Copyright (c) 2018 Paul van Haastrecht.
 *
 * This javascript runs on nodejs and will setup a server that communicates
 * between network program (e.g. browser) and a ThingMagic reader program
 * created with the MercuryAPI.
 *
 * This has been tested under the following conditions
 *   Ubuntu 16.4 LTS / Raspberry Pi (Jessie)
 *   nodejs v4.2.6 / V9.9.0
 *   MercuryAPI 29.4.34
 *   Modified readall.c example
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Paulvh : March 2018
 * Initial version.
 */

var http = require('http');
var url = require('url');
var fs = require('fs');

// definitions
var write_pipe = './tmr_to'        // pipename definitions (can be overruled from command line)
var read_pipe = './tmr_from'
var OK_TIMEOUT = 2;                // default timeout after sending command (2 seconds)
var RESP_TIMEOUT = 5;              // default timeout wait on result of command (5 seconds)
var VERBOSE = 0;                   // display debug message (can be overruled from command line)
var MAXLOOPCOUNT = 150;            // prevent deadloop ( ~ 150 x OK_TIMEOUT = 300 sec = 5 min)

// working variables
var error = 0;                      // indicate we are displaying error message
var loopcount = 0;                  // keep track of timeout(s)
var res_save = 0;                   // save the handle to the result socket
var r_set = 0;                      // indicator read stream has been initialized
var time_out = 0;                   // timeout counter
var wait_ack = 0;                   // expect feedback 1 = echo command, 2 = result or command
var req_command = " ";              // requested command for TMR
var parse_result = "";              // feedback parsing return from TMR
var parse_length=0;                 // length of parse_result
var save_cmd="";                    // save command for (potential) resend
var error_message="";               // display error message to remote.
var intervalobj=0;                  // interval handle
var r_stream;                       // pipe handlers
var w_stream;


// parse parameters
var n = 2;
while (n < process.argv.length)
{
    // overrule default read from TMR
    if (process.argv[n] == "readpipe")
    {
        n++;
        read_pipe = process.argv[n];
    }

    // overrule default write TO TMR
    else if (process.argv[n] == "writepipe")
    {
        n++;
        write_pipe = process.argv[n];
    }
    // enable verbose debug messages
    else if (process.argv[n] == "verbose")
    {
        VERBOSE = 1;
    }
    // set for error message
    else if (process.argv[n] == "error")
    {
        n++;
        
        // indicate we display error message
        error = 1;
        error_message = process.argv[n]
    }
    else
    {
        console.log("usage " + process.argv[1] + " [writepipe pipename] [readpipe pipename] [verbose] [error errormessage]\n");
        process.exit(2);
    }

    n++;
}

// if no error display provided (this is normal operation)
if(error == 0)
{
    // sent sign of live (even if verbose is not set)
    console.log("starting Nodejs script writepipe " + write_pipe + " read pipe " + read_pipe );
  
    // open pipes
    try
    {
        verbose("connecting pipes");
        r_stream = fs.createReadStream(read_pipe);
        w_stream = fs.createWriteStream(write_pipe);
    }
    catch(err) {
        console.log('problem during opening pipes '+ err + '\n');
        process.exit(2);
    }
}
// exceptional operation to display error message in case something wrong
// to inform the remote user
else
{
    console.log("starting Nodejs script to display error message : " + error_message);
}

/** display progress / verbose messages if requested
 ** change timezone to where ever you are in the world 
 **/
function verbose(mess) {

    if (VERBOSE == 1){
       // process.env.TZ = 'Europe/Amsterdam'
        console.log("nodejs: "+ new Date().toString() + " " + mess);
    }
}

/** sleep function
 ** time : sleep time milliseconds 
 **/
function sleep_m(time) {
    var stop = new Date().getTime();
    while(new Date().getTime() < stop + time) { ; }
}

/** sent responds back browser or remote program
 **
 ** this is the place to customize a different responds format
 ** to your needs for the remote program
 **/
function handle_responds(responds)
{
    // if empty responds
    if (! responds) return;

    // the TMR reader stopped
    if (responds.indexOf("!STOPPING!") > -1)  
    {
        verbose('received stop from TMR');

        // inform the requested
        res_save.end('Error: 7f00, Reader has stopped. Will try to restart' + '<br>');

        // wait to make sure this is sent
        sleep_m(5);

        // exit with status 0
        process.exit(0);
    }
    else
    {
        /// write results to caller in HTML
        /// here to do some different packaging of the data
        res_save.end(responds + '<br>');

        // reset req_command
        req_command = "";
    }
}

/** handle responds from TMR
 ** find needle in haystack(buf)
 **
 ** return value in global parse_result / parse_length 
 **/
function pars_responds(buf, needle)
{
    parse_result = "";
    parse_length = 0;
    var n;
    var s_buf = buf;        // search buffer

    /// check whether there are 2 {} {} response in same responds
    // look for end first.. should always be found
    n = buf.indexOf('}');

    // if found (and it better be...)
    if (n > -1)
    {
        // make sub string
        var st_next = buf.slice(n, buf.length);

        // look for start of potential second
        n = st_next.indexOf('{');

        // if there is another message we got the response immediately
        if (n > -1)
        {
            wait_ack = 2;       // skip waiting on feedback
            s_buf = st_next;    // set to start of additional responds
        }
    }

    /// find the requested value in the search buf
    // look for entry
    n = s_buf.indexOf(needle);

    if (n > -1)
    {
        // +3 to skip ":"
        var st_val = s_buf.slice(n + needle.length + 3, buf.length);

        // look for closing "
        n = st_val.indexOf('\"');

        if (n > -1)
        {
            parse_result = st_val.slice(0, n);
            parse_length = n;
        }
    }
}

/** read from TMR pipe
 **
 ** wait_ack is checked :
 ** 0 : not expecting anything
 ** 1 : was waiting on command to return from TMR on command
 ** 2 : was waiting on result of command execution 
 **/
function read_from_tmr(){
 
    // did the pipe close on the other side ?
    r_stream.on('end',function(){

        verbose("end of TMR / read pipe");

        // inform remote program/browser
        handle_responds("Error: connection closed !RESTART!");

        // if combined with the shell script it will restart
        process.exit(3);
    });

    r_stream.on('error', function(err){
        console.log("error reading pipe");
    });
    
    r_stream.on('close', function() {
        
        verbose("close of TMR / read pipe");

        // inform remote program/browser
        handle_responds("Error: connection timeout !RESTART!");

        // if combined with the shell script it will restart
        process.exit(3);
    });

    // data received
    r_stream.on('data', (d) => {

        // debug to screen
        verbose('got from TMR: ' + d.toString());

        // get the value returned
        pars_responds(d.toString(),"val");

        // if value found
        if(parse_length > 0)
        {
            verbose("wait_ack "+ wait_ack);

            // expecting ACK after sending command
            if (wait_ack == 1)
            {
                verbose('Length message: ' + parse_length + ' command length: '  + req_command.length);

                // only check the length
                if (parse_length == req_command.length)
                {
                    // set timeout to wait for response on command
                    time_out = RESP_TIMEOUT;

                    // indicate waiting on command result
                    wait_ack = 2;
                    verbose('wait on result of command execution');
                }
            }

            // response received from command
            else if (wait_ack == 2)
            {
                // got response on command
                wait_ack = 0;

                // save status
                var ret_val = parse_result;

                // get result information
                pars_responds(d.toString(),"ret");

                if (parse_length > 0)
                {
                    verbose("Message status " + parse_result);

                    // if not TMR_SUCCESS
                    if (parse_result != 0)
                    {
                        verbose("Error: " + parse_result + ' ' + ret_val);
                        var ret_string ="Error:" + parse_result + "," + ret_val;
                    }
                    else
                    {
                        verbose('Result: '+ parse_result + ' ' + ret_val);
                        var ret_string ="Result:"+ parse_result + "," + ret_val;
                    }
    
                    // sent result string
                    handle_responds(ret_string);
                }
            }

            // else discard what was received. Unwanted rubbish from earlier
        }
    })
}

/** sent command to TMR and indicate we expect OK back
 ** command : command to sent to TMR 
 **/
function sent_to_tmr(command)
{
    // sent to TMR + readtimeout / retry etc. etc.
    // ALWAYS UPPERCASE. in case the user makes an error
    w_stream.write(command.toUpperCase());
 
    verbose ("sending to TMR: " + command)

    // set timeout counter
    time_out = OK_TIMEOUT;

    // indicate pending command acknowledgement
    wait_ack = 1;

    // set timeout check every 1 seconds
    intervalobj = setInterval(handle_timeout, 1000);
}

/** check for timely responds 
 **
 ** the loopcount is used to make sure in case retry=0 was set
 ** and the reader does not respond after MAXLOOPCOUNT seconds 
 ** the operation is still terminated.
 **/
function handle_timeout(){

    /* if we wait on ACK from command (1) or the results of request (2)
     * perform time_out check.
     */

    if (wait_ack > 0)
    {
        verbose("timeout " + time_out + " loopcount "+ loopcount);

        // if timed out
        if (0 > --time_out)
        {
           // clear interval (will be setin sent_to_tmr()
           clearInterval(intervalobj);
           
           // increase loop count
           loopcount++;

           // if maximum reached
           if (loopcount > MAXLOOPCOUNT) 
           {
               // tel the reader to cancel
               w_stream.write('CMD=CANCEL');
               process.exit(3);
           }

           // sent command again
           sent_to_tmr(save_cmd);
        }
    }
    else
    {
        // clear interval (not needed anymore)
        clearInterval(intervalobj);
        
        // reset timeout counter
        loopcount = 0;
    }
}
/*********************************
 ** Here the real work starts   **
 *********************************/
http.createServer((req, res) => {
    
    // normal operation
    if (error == 0)
    {
          res.writeHead(200, {'Content-Type': 'text/html'});
        
          // parse the URL
          var q = url.parse(req.url, true);
        
          // get the TMR search command(s) to parse
          req_command = q.search
        
          // save the responds constructor to sent return from TMR reading
          res_save = res;
        
          try
          {
            // if received command is not empty
            if (req_command)
            {
                verbose('received TMR instruction(s) ' + req_command);
        
                // Initiate reading thread from the pipe back from TMR only once
                if (r_set == 0)
                {
                    r_set = 1;
        
                    verbose('Initialize read-thread');
        
                    read_from_tmr();
                }
        
                // if remote wants to cancel current operation
                if(req_command.indexOf("cancel") > -1 || req_command.indexOf("CANCEL") > -1 )
                {
                    verbose('received request to cancel operation');
        
                    // let the reader program know
                    w_stream.write('CMD=CANCEL');

                    // let the remote user know
                    handle_responds("Result 0, cancelled");
                
                    // restart the programs (when connected to the shell script)
                    process.exit(3);
                }
                else
                {
                    // write command to TMR
                    sent_to_tmr(req_command);
        
                    // save received command from online for (potential) resend
                    save_cmd = req_command;
                }
            }
          }
        
          catch(err) {
            console.log('problem during handling request '+ err + '\n');
          }
        }
        else     // display error message only (forever)
        {
           res.writeHead(400, {'Content-Type': 'text/html'});
           res.end("Error: 7F01, " + error_message);
        }

}).listen(8080);
