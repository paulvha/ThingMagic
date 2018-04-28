/*
 * Copyright (c) 2018 Paul van Haastrecht.
 *
 * ThingMagic, a JADAK Brand. All Rights Reserved.
 *
 * This javascript runs on nodejs and can receive, parse and display the data
 * as provided by ThingMagic universal reader Assistant (URA) / Data Extensions / HTTP POST
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
 * Paulvh : April 2018
 * Initial version.
 */

var http = require('http');
var qs = require('querystring');

// save entry information
var entry_result;
var entry_length;

/* extract entry from Tag_line
 *
 * Input:
 *   buf : TAG-line
 *   buf_length : length of TAG_line
 *   entry : entry to extrac ( 1 ..n).
 *           an Entry is terminate by eiter comma or \r (0x0d)
 *
 * Return :
 *  set entry_result with entry content or empty if failed.
 *  set entry_length with length of content or -1 if failed.
 */

function get_entry(buf, buf_length, entry)
{
    var start_ent = 0 ;
    var cnt = 0;
    var counter = 0;

    entry_result = "";

    while (counter <= buf_length)
    {
        // each entry is terminated with , or backspace (last)
        if (buf[counter] == ',' || buf[counter] == '\r')
        {
            // count entry
            cnt++;

            // reached the right entry ?
            if (cnt == entry)
            {
                entry_length = counter - start_ent;
                return;
            }

            // reset for next entry
            else
            {
                entry_result = "";
                start_ent = counter + 1;
            }
        }
        else  // add content to entry_result
        {
            entry_result += buf[counter];
        }

        counter++;
    }

    // not found reset
    entry_length = -1;
    entry_result = "";
}

/* parse received tag information
 * input:
 *  values : all the field_values from the content
 * */
function parse_tags_info(values)
{
    var length = values.length;
    var start_val = 0;
    var buf = "";

    // debug / display field values
    console.log("field_values "+ "\n" + values);

    // loop through value. It ends with 0x0a, 0x0a
    while (start_val < length)
    {
        // add to tmp buffer
        buf += values[start_val];

        // extract each TAG-line
        if (values[start_val] == '\n')
        {
            // parse information for each TAG-entry
            // get EPC
            get_entry(buf, buf.length, 1);
            if (entry_length != -1)
                var EPC = entry_result;
            else
                var EPC = "NOT FOUND";

            // get TimeStamp
            get_entry(buf, buf.length, 2);
            if (entry_length != -1)
            {
                var TS = entry_result;

                // demonstrate how to split time and date
                var time = TS.split(" ")[1];
                var date = TS.split(" ")[0];
            }
            else
            {
                var TS = "NOT FOUND";
                var time = "NOT FOUND";
                var date = "NOT FOUND";
            }

            // get RSSI
            get_entry(buf, buf.length, 3);
            if (entry_length != -1)
                var RSSI = entry_result;
            else
                var RSSI = "NOT FOUND";

            // get ReadCount
            get_entry(buf, buf.length, 4);
            if (entry_length != -1)
                var CNT = entry_result;
            else
                var CNT = "NOT FOUND";

            // debug / display field values
            console.log("EPC " + EPC);
            console.log("TimeStamp "+ TS);
            console.log("Date "+ date + " Time " + time);
            console.log("RSSI " + RSSI);
            console.log("Count " + CNT);

            // Here save the values you want to the database

            // reset buffer
            buf = "";
        }

        start_val++;
    }
}

http.createServer((req, res) => {

    if (req.method == 'POST') {
        var body = '';

        req.on('data', function (data) {
            body += data;
        });

        req.on('end', function () {

            // console.log("Total content: " + body);

            var POST = qs.parse(body);

            // only get field_values
            parse_tags_info(POST.field_values);

        });

        // sent response (otherwise the URA stalls)
        res.writeHead(200, {'Content-Type': 'text/html'});
        res.end();
    }

}).listen(8080);
