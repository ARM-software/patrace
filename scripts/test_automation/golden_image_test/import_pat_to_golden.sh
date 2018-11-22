RETRACER=$3

$RETRACER -info $1> info

tid=`grep "default tid" info | cut -d' ' -f4`
red=`grep "red bits" info | cut -d' ' -f7`
blue=`grep "blue bits" info | cut -d' ' -f6`
green=`grep "green bits" info | cut -d' ' -f5`
alpha=`grep "alpha bits" info | cut -d' ' -f5`
stencil=`grep "stencil bits" info | cut -d' ' -f3`
depth=`grep "depth bits" info | cut -d' ' -f5`
width=`grep "winWidth" info | cut -d' ' -f6`
height=`grep "winHeight" info | cut -d' ' -f5`
rm info

#if [ "x"$2 != "x" ] && [ "x"$3 != "x" ]; then
#    width=$2;
#    height=$3;
#fi
snaps=50
if [ "x"$2 != "x" ]; then
    snaps=$2
fi

json_name=`echo $1 | sed s/.pat/.json/`
basename=`echo $1 | sed s/.pat//`
dirpath=`pwd`
dirname=`echo $dirpath | awk -F/ '{print $NF}'`

cat > $json_name <<EOF
{
    "__include__" : [ "../golden_generic.json"],
    
    "result_parser":  {
	"com.arm.pa.cmdlistplugin.golden_image_parser": {
	    "golden_repo_dir": "$dirname" }
	},
    "tracefile": "$1",
    "files":[{
	"file": "$1",
	"path": "$dirpath/$1"
    }],
    "snapshotCallset": "frame/*/$snaps",
    "colorBitsRed": $red,
    "colorBitsBlue": $blue,
    "colorBitsGreen": $green,
    "colorBitsAlpha": $alpha,
    "stencilBits": $stencil,
    "depthBits": $depth,
    "tid": $tid,
    "winW": $width,
    "winH": $height
}
EOF
