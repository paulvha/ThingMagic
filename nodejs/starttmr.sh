#! /bin/bash
# 
# start nodejs server and TMR reader
#
# To be used in combination with tmr.js / read_nodejs.c
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# version 1.0 / paulvha / March 2018


################################################
###      BEGIN PARAMETERS SETTING            ###
################################################

# For progress information in the log files remove the hash sign
#VERBOSE="1"

# For deep detailed ( progress + low level communication with the reader)
# remove the hash sign
VERBOSE="2"

# device information
TMR_PREFIX="tmr://"
DEVICE="/dev/ttyUSB0"

# logfiles
TMRPRG_LOG="/tmp/tmrprog.log"
NODEPRG_LOG="/tmp/nodeprog.log"

# pipenames to use
P_TO_TMR="./tmr_to"
P_FROM_TMR="./tmr_from"

# programs to use
nodejs="nodejs"
nodeprg="tmr.js"
tmrprg="./read_nodejs"

################################################
###        END PARAMETERS SETTING            ###
################################################

# Make sure only root can run the script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root"
   exit 1
fi

# Check that pipes exist, otherwise create them
if ! [ -p $P_TO_TMR ]
then
    echo "$P_TO_TMR will be created"

    mkfifo $P_TO_TMR

    if ! [ $? -eq 0 ]; then
        echo "$P_TO_TMR can not be created"
        exit 1
    fi
fi

if ! [ -p $P_FROM_TMR ]
then
    echo "$P_FROM_TMR will be created"

    mkfifo $P_FROM_TMR

    if ! [ $? -eq 0 ]; then
        echo "$P_FROM_TMR can not be created"
        exit 1
    fi
fi

# catch signals
sigquit()
{
    echo "signal to stop received"
    kill $TMR_PID >/dev/null 2>&1
    exit 0
}


trap 'sigquit' QUIT
trap 'sigquit' INT


# Main loop server (will loop as long as return code is 3)
out=3

while [ $out -eq 3 ]
do

    # check that DEVICE exists
    if ! [ -c $DEVICE ] ; then
    
        #let users know who try to log on as well
        MESS=`echo "Device $DEVICE not available"`
        echo $MESS
        $nodejs $nodeprg error "$MESS"
        
        exit 2
    fi

    # start TMR reader program in background
    if [ -z ${VERBOSE+x} ];
    then
        # verbose was NOT set
        $tmrprg -r $P_TO_TMR -w $P_FROM_TMR $TMR_PREFIX$DEVICE > $TMRPRG_LOG &
    else
        # verbose was set (either 1 = progress, 2 = 1 + low level tracing)
        $tmrprg -v $VERBOSE -r $P_TO_TMR -w $P_FROM_TMR $TMR_PREFIX$DEVICE > $TMRPRG_LOG &
    fi

    # after successful starting TMR reader program
    # now start the nodejs program
    if [ $? -eq 0 ];
    then
        # save PID
        TMR_PID=$!

        echo "Ready to connect" 
        
        # start NODEJS
        if [ -z ${VERBOSE+x} ];
        then
            # verbose was NOT set
            $nodejs $nodeprg writepipe $P_TO_TMR readpipe $P_FROM_TMR > $NODEPRG_LOG
        else
            # verbose was set
            $nodejs $nodeprg verbose writepipe $P_TO_TMR readpipe $P_FROM_TMR > $NODEPRG_LOG
        fi

        # Get return code
        out=$?

        # stop the TMR program (in case it was still running)
        kill $TMR_PID >/dev/null 2>&1
    else
        echo "could not start $tmrprg $DEVICE"
        out=2
    fi
done

exit $out
