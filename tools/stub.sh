#!/bin/sh

hash1=__HASH1__
hash2=__HASH2__
skip=__SKIP__
skip1=__SKIP1__
command=__COMMAND__
log=/tmp/nxagentupdate.log

trap '
	echo "Upgrade script finished" >> $log
	cd $wd
	[ "x"$temp != "x" ] && rm -rf $temp
	exit
' INT EXIT

echo "$0 started: $*" > $log

wd=`pwd -P`
echo "Working directory: $wd" >> $log

_mktemp() {
	d=/tmp/nxupdate.$$.UniQ
	mkdir $d && echo $d || false
}

if [ "x`echo test | md5 2>/dev/null | tr A-Z a-z | cut -b1-32`" = 'xd8e8fca2dc0f896fd7cb4cb0031ba249' ]; then
   md5="md5 | tr A-Z a-z | cut -b1-32"
fi
if [ "x$md5" = "x" ] && [ "x`echo test | md5sum 2>/dev/null | tr A-Z a-z | cut -b1-32`" = 'xd8e8fca2dc0f896fd7cb4cb0031ba249' ]; then
   md5="md5sum | tr A-Z a-z | cut -b1-32"
fi
if [ "x$md5" = "x" ] && [ "x`echo test | csum -h MD5 - 2>/dev/null | tr A-Z a-z | cut -b1-32`" = 'xd8e8fca2dc0f896fd7cb4cb0031ba249' ]; then
   md5="csum -h MD5 - | tr A-Z a-z | cut -b1-32"
fi
if [ "x$md5" = "x" ] && [ "x`echo test | openssl md5 2>/dev/null | tr A-Z a-z`" = 'xd8e8fca2dc0f896fd7cb4cb0031ba249' ]; then
   md5="openssl md5 | tr A-Z a-z"
fi
if [ "x$md5" = "x" ] && [ "x`echo test | openssl md5 2>/dev/null | tr A-Z a-z | rev | cut -b1-32 | rev`" = 'xd8e8fca2dc0f896fd7cb4cb0031ba249' ]; then
   md5="openssl md5 | tr A-Z a-z | rev | cut -b1-32 | rev"
fi

tail="tail -n"
case `uname -s` in
	SunOS)
		tail="tail"
		mktemp() {
			_mktemp $*
		}
		;;
	HP-UX)
		alias mktemp=_mktemp
		;;
	*)
		;;
esac

tmp=`which mktemp 2>/dev/null`
if [ $? = 0 ]; then
	echo $tmp | grep "no mktemp in " >/dev/null 2>&1
	[ $? = 0 ] && alias mktemp=_mktemp
else
	alias mktemp=_mktemp
fi

if [ "x$md5" != "x" ]; then
	if [ "X"`head -n $skip $0 |
		$tail +5 |
		$md5` != "X"$hash1 ];
	then
		echo "Script MD5 mismach; upgrade aborted" >> $log
		exit
	fi

	if [ "X"`$tail +$skip1 $0 |
		$md5` != "X"$hash2 ];
	then
		echo "Payload MD5 mismach; upgrade aborted" >> $log
		exit
	fi
else
	echo "No md5/md5sum/csum/openssl found, can't check integrity" >> $log
fi

temp=`mktemp -d /tmp/nxinst.XXXXXX 2>/dev/null`
if [ $? != 0 ]; then
	echo "Can't create temp dir" >> $log
	exit
fi
cd $temp
$tail +$skip1 $wd/$0 | gzip -dc 2>/dev/null | tar xf -
if [ $? != 0 ]; then
	echo "Can't unpack" >> $log
	exit;
else
	chmod +x ./$command
	if [ $? != 0 ]; then
		echo "Can't chmod $command" >> $log
		exit
	fi
	echo Starting $command $*... >> $log
	./$command $* 2>&1 >> $log
fi

exit
###################################
