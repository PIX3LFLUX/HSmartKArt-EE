#!/bin/bash

# style reset
STYLE_RESET='\e[0m'
# red foreground color
STYLE_RED_FORE='\e[31m'
# blue foreground color
STYLE_BLUE_FORE='\e[34m'

#usage()
##{
#	echo -e "Usage: $0 [-d|--domain mydomain.com|ip-address]"
#	echo -e "\tA domain or an IP address on which the server is accessible needs to be provided as argument"
#	exit 1
##}

# Check command line arguments
#while [ "${1:-}" != "" ]
#do
#	case "$1" in
#		-d|--domain)	shift # shift moves arguments position down by one
#						SERVER_DOMAIN="$1"
#						;;
#		* )				usage
#						;;
#	esac
#	shift
#done

# --- System Updates --- #
apt update
apt full-upgrade -y

# --- Firewall (ufw) --- #
ufw enable
ufw status
ufw app list
ufw allow OpenSSH

# --- Install certbot --- #
apt remove -y certbot
snap install core
snap refresh core
snap install --classic certbot
ln -s /snap/bin/certbot /usr/bin/certbot

# --- Install Nginx --- #
apt install -y curl gnupg2 ca-certificates lsb-release
echo "deb http://nginx.org/packages/ubuntu `lsb_release -cs` nginx" | tee /etc/apt/sources.list.d/nginx.list
echo -e "Package: *\nPin: origin nginx.org\nPin: release o=nginx\nPin-Priority: 900\n" | tee /etc/apt/preferences.d/99nginx
curl -o /tmp/nginx_signing.key https://nginx.org/keys/nginx_signing.key
gpg --dry-run --quiet --import --import-options show-only /tmp/nginx_signing.key
mv /tmp/nginx_signing.key /etc/apt/trusted.gpg.d/nginx_signing.asc
apt update
apt install -y nginx-full

# --- Setup firewall for Nginx --- #
ufw allow 'Nginx Full'

# --- Create and install certificate for Nginx --- #
if [ ! -a /etc/letsencrypt/renewal/mqttee-server.conf ]
then
	certbot --nginx
	cat >> /etc/letsencrypt/renewal/mqttee-server.conf <<EOT

renew_hook = systemctl reload jupyterlab
EOT
	certbot renew --dry-run
	chmod 750 -R /etc/letsencrypt
	chown $USER:$USER -R /etc/letsencrypt
else
	echo -e "Certificate already generated. Skipping to next Installation step\n"
fi

# --- Configure Nginx --- #
if [ ! -a /etc/nginx/sites-enabled/jupyter.conf ]
then
	cp /etc/nginx/sites-enabled/default /etc/nginx/sites-available/
	rm /etc/nginx/sites-enabled/default
	cp config/jupyter.conf /etc/nginx/sites-available/
	ln -s /etc/nginx/sites-available/jupyter.conf /etc/nginx/sites-enabled/
else
	echo -e "Nginx configuration already done. Skipping to next Installation step\n"
fi

# --- Install Mosquitto --- #
apt install -y git bc libncurses5-dev bc build-essential ccache cmake libssl-dev
apt install -y mosquitto mosquitto-clients

# --- Install and activate Miniconda3 --- #
if [ ! -d $HOME/miniconda3 ]
then
	wget -P /tmp https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
	bash /tmp/Miniconda3-latest-Linux-x86_64.sh
	source ~/.bashrc
else
	echo -e "Miniconda alread installed and activated. Skipping to next Installation step\n"
fi

# --- Install JupyterLab --- #
conda install -c conda-forge jupyterlab
apt install -y nodejs

# --- Configure JupyterLab --- #
if [ ! -d $HOME/.jupyter ]
then
	jupyter lab --generate-config
	jupyter lab password
	cp config/jupyter_lab_config.py .jupyter/
else
	echo -e "JupyterLab configuration already done. Skipping to next Installation step\n"
fi

# --- Setup Systemd Service --- #
if [ ! -a /etc/systemd/system/jupyterlab.service ]
then
	cat >> /etc/systemd/system/jupyterlab.service <<EOT
[Unit]
Description=Jupyter Lab Server

[Service]
User=$USER
Group=$USER
WorkingDirectory=$HOME/
ExecStart=$HOME/miniconda3/bin/jupyter-lab
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOT

else
	echo -e "JupyterLab systemd service setup already done. Skipping to next Installation step\n"
fi

# --- Install Paho MQTT --- #
if [ ! -a $HOME/miniconda3/pkgs/paho-mqtt* ]
then
	conda install -c conda-forge paho-mqtt
else
	echo -e "Paho MQTT Python library already installed. Skipping to next Installation step\n"
fi

echo -e "${STYLE_BLUE_FORE}\n\nInstalltion script successfully executed.${STYLE_RESET}\n"

echo -e "${STYLE_RED_FORE}\n\nModify /etc/nginx/sites-available/jupyter.conf and ~/.jupyter/jupyter_lab_config.py and then reboot system to finish installation.${STYLE_RESET}\n"

exit 0
