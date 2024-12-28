#!/bin/bash

# Detect user and computer name
USER_NAME=$(whoami)
COMPUTER_NAME=$(hostname)

INSTALL_DIR="/usr/local/bin"
SERVICE_FILE="/etc/systemd/system/littlewin.service"

install_dependencies() {
  echo "Detecting distribution and installing dependencies..."

  local dependencies="libx11-dev cmake gcc make"

  if [ -f /etc/os-release ]; then
    . /etc/os-release
    case "$ID" in
    ubuntu | debian)
      echo "Detected Debian-based distribution."
      sudo apt update
      for dep in $dependencies; do
        if dpkg-query -l "$dep" &>/dev/null; then
          echo "$dep is already installed."
        else
          echo "Installing $dep..."
          sudo apt install -y "$dep"
        fi
      done
      ;;
    rhel | fedora | centos | almalinux | rocky)
      echo "Detected RHEL-based distribution."
      sudo dnf check-update || sudo yum check-update
      sudo dnf install -y libX11-devel cmake gcc make || sudo yum install -y libX11-devel cmake gcc make
      ;;
    arch | manjaro)
      echo "Detected Arch-based distribution."
      sudo pacman -Syu --noconfirm
      sudo pacman -S --noconfirm libx11 cmake gcc make
      ;;
    *)
      echo "Unsupported distribution: $ID"
      exit 1
      ;;
    esac
  else
    echo "Unable to detect distribution. Ensure dependencies are installed manually."
    exit 1
  fi
}

build_and_install() {
  echo "Building the project..."
  cmake .
  make

  echo "Installing littlewin to $INSTALL_DIR..."
  sudo cp littlewin "$INSTALL_DIR/"
  sudo chmod +x "$INSTALL_DIR/littlewin"
}

create_systemd_service() {
  echo "Creating systemd service for littlewin..."

  if [ -f "$SERVICE_FILE" ]; then
    echo "Service already exists. Removing old service..."
    sudo systemctl stop littlewin.service
    sudo systemctl disable littlewin.service
    sudo rm "$SERVICE_FILE"
  fi

  sudo bash -c "cat > $SERVICE_FILE" <<EOL
[Unit]
Description=littlewin window manager
After=graphical.target

[Service]
ExecStart=$INSTALL_DIR/littlewin
User=$USER_NAME
Environment=DISPLAY=:1
Environment=XDG_SESSION_TYPE=$XDG_SESSION_TYPE   # Pass the session type environment variable
Restart=always
RestartSec=3

[Install]
WantedBy=default.target
EOL

  sudo systemctl daemon-reload
  sudo systemctl enable littlewin.service
  sudo systemctl start littlewin.service
}

activate_dynamic_workspaces() {
  if [ "$XDG_CURRENT_DESKTOP" == "GNOME" ]; then
    echo "GNOME desktop detected. Activating dynamic workspaces..."
    gsettings set org.gnome.mutter dynamic-workspaces true
    echo "Dynamic workspaces activated."
  else
    echo "Dynamic workspaces configuration is not applicable for $XDG_CURRENT_DESKTOP."
  fi
}

echo "Starting installation of littlewin..."

install_dependencies
build_and_install
create_systemd_service
activate_dynamic_workspaces

echo "Installation complete! littlewin is set to run at login."
