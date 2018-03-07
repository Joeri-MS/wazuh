#!/bin/sh
################################################################################
# Wazuh-Vuls integration deployment script
# Wazuh Inc.
# Dec 19, 2017
################################################################################

OS_NAME=$1
OS_VER=$2
PROXY=$3

if [ "x$OS_NAME" == "x" -o "x$OS_VER" == "x" ];then
    echo "  Use: ./deploy <OS> <version>"
    echo
    echo "  Allowed values:"
    echo "          - sles 11, sles 12"
    echo "          - opensuse 42.2, opensuse 42.3"
    echo "          - redhat 7, redhat 6, readhat 5"
    echo "          - centos 7, centos 6, centos 5"
    echo "          - ubuntu 16, ubuntu 14, ubuntu 12"
    echo "          - debian 10, debian 9, debian 8, debian 7"
    echo "          - oracle 7, oracle 6, oracle 5"
    exit 1
fi

WAZUH=`cat /etc/ossec-init.conf | head -1 | cut -d '"' -f 2 | sed 's/ossec\//ossec/'`
INSTALL_PATH=$WAZUH"/wodles/vuls"
LOG_PATH=$WAZUH"/logs/vuls"
VULS_AGENT=$INSTALL_PATH"/vuls.py"

echo "  OS NAME: $OS_NAME"
echo "  OS VERSION: $OS_VER"
VUL_PROXY=""
if [ "x$http_proxy" != "x" ];then
   echo "  Proxy: $http_proxy"
   VUL_PROXY="--proxy $http_proxy"
   echo "Pleas check our /etc/wgetrc and ~/.gitconfig for Proxy settings ;)"
fi

THREADS=$(grep processor /proc/cpuinfo | wc -l)

echo
echo "********** Step1. Install requirements **********"
echo

if [ "$OS_NAME" = "redhat" ] || [ "$OS_NAME" = "centos" ] || [ "$OS_NAME" = "oracle" ]; then
    if [ "$OS_NAME" = "centos" ]; then
        OS_NAME="redhat"
    fi
    sudo yum -y install python which sqlite git gcc make wget yum-utils
elif [ "$OS_NAME" = "ubuntu" ] || [ "$OS_NAME" = "debian" ]; then
    sudo apt -y install python which sqlite git gcc make wget reboot-notifier
elif [ "$OS_NAME" = "sles" ];then
    sudo zypper in -y python which sqlite3 git-core gcc make wget 
elif [ "$OS_NAME" = "opensuse" ]; then
    sudo zypper in -y python which sqlite3 git gcc make wget 
else
    echo "  Enter a valid OS"
    exit 1
fi

PYTHON_PATH=`which python`

wget -O go1.8.3.linux-amd64.tar.gz https://storage.googleapis.com/golang/go1.8.3.linux-amd64.tar.gz
sudo tar -C /usr/local -xzf go1.8.3.linux-amd64.tar.gz

export GOROOT=/usr/local/go
export GOPATH=$INSTALL_PATH/go
export PATH=$PATH:$GOROOT/bin:$GOPATH/bin

echo
echo "********** Step2. Deploy CVE dictionary **********"
echo

mkdir -p $GOPATH/src/github.com/kotakanbe
cd $GOPATH/src/github.com/kotakanbe
rm -rf go-cve-dictionary
git clone https://github.com/kotakanbe/go-cve-dictionary.git
cd go-cve-dictionary
make -j$THREADS install

$PYTHON_PATH $VULS_AGENT --updatenvd --onlyupdate --nvd-year 2002 $VUL_PROXY

echo
echo "********** Step3. Deploy goval-dictionary **********"
echo

mkdir -p $GOPATH/src/github.com/kotakanbe
cd $GOPATH/src/github.com/kotakanbe
rm -rf goval-dictionary
git clone https://github.com/kotakanbe/goval-dictionary.git
cd goval-dictionary
make -j$THREADS install

if [ "$OS_NAME" = "sles" ]; then
    $PYTHON_PATH $VULS_AGENT --updatesles --os-version $OS_VER --onlyupdate $VUL_PROXY
elif [ "$OS_NAME" = "opensuse" ]; then
    $PYTHON_PATH $VULS_AGENT --updatesuse --os-version $OS_VER --onlyupdate $VUL_PROXY
elif [ "$OS_NAME" = "redhat" ]; then
    $PYTHON_PATH $VULS_AGENT --updaterh --os-version $OS_VER --onlyupdate $VUL_PROXY
elif [ "$OS_NAME" = "ubuntu" ]; then
    $PYTHON_PATH $VULS_AGENT --updateub --os-version $OS_VER --onlyupdate $VUL_PROXY
elif [ "$OS_NAME" = "debian" ]; then
    $PYTHON_PATH $VULS_AGENT --updatedeb --os-version $OS_VER --onlyupdate $VUL_PROXY
elif [ "$OS_NAME" = "oracle" ]; then
    $PYTHON_PATH $VULS_AGENT --updateorac --os-version $OS_VER --onlyupdate $VUL_PROXY
fi

echo
echo "********** Step4. Deploy Vuls *********"
echo

mkdir -p $GOPATH/src/github.com/future-architect
rm -rf $GOPATH/pkg/linux_amd64/github.com/future-architect/vuls/
rm -rf $GOPATH/src/github.com/future-architect/vuls/
cd $GOPATH/src/github.com/future-architect
git clone https://github.com/future-architect/vuls.git
cd vuls
make -j$THREADS install

echo
echo "********** Step5. Configuration *********"

cd $INSTALL_PATH
cat > config.toml <<\EOF
[servers]

[servers.localhost]
host = "localhost"
port = "local"
EOF

echo
echo "********** Step6. Check config.toml and settings on the server before scanning *********"
echo

vuls configtest -log-dir $LOG_PATH
