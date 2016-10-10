export PATH=$PATH:/home/laurie/gcc-arm/bin
export CC_DIR=/home/laurie/gcc-arm
export ARMGCC_DIR=/home/laurie/gcc-arm

fr=/home/laurie/RTES/KSDK_1.3.0/examples/frdmkl46z

alias screenCortex='sudo screen /dev/ttyACM0 115200'

function cpToCortex {
	if [ -z "$2" ]
	then
		DEST=/run/media/laurie/FRDM-KL46Z/
	else
		DEST=$2
	fi
	
	if [ -z "$1" ]
	then
		echo "Finding .srec file..."	
		SRC=$(find build/*.srec)
		echo $SRC
	else
		SRC=$1
	fi

	sudo cp $SRC $DEST	
}

function objcpy_armgcc {
	if [ $# -ne 2 ]
	then
		echo "Not enough arguments"
		exit
	fi
	arm-none-eabi-objcopy -O srec $1 $2
}

function uploadCortex {
	if [ $# -eq 0 ]
	then
		echo "Not enough arguments"
		exit
	fi
	objcpy_armgcc $1 temp.srec
	cpToCortex temp.srec
	rm temp.srec
}

#
# ~/.bashrc
#

[[ $- != *i* ]] && return

colors() {
	local fgc bgc vals seq0

	printf "Color escapes are %s\n" '\e[${value};...;${value}m'
	printf "Values 30..37 are \e[33mforeground colors\e[m\n"
	printf "Values 40..47 are \e[43mbackground colors\e[m\n"
	printf "Value  1 gives a  \e[1mbold-faced look\e[m\n\n"

	# foreground colors
	for fgc in {30..37}; do
		# background colors
		for bgc in {40..47}; do
			fgc=${fgc#37} # white
			bgc=${bgc#40} # black

			vals="${fgc:+$fgc;}${bgc}"
			vals=${vals%%;}

			seq0="${vals:+\e[${vals}m}"
			printf "  %-9s" "${seq0:-(default)}"
			printf " ${seq0}TEXT\e[m"
			printf " \e[${vals:+${vals+$vals;}}1mBOLD\e[m"
		done
		echo; echo
	done
}

[[ -f ~/.extend.bashrc ]] && . ~/.extend.bashrc

[ -r /usr/share/bash-completion/bash_completion   ] && . /usr/share/bash-completion/bash_completion

