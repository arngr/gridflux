#!/bin/bash

# Detect user and computer name
USER_NAME=$(whoami)
COMPUTER_NAME=$(hostname)

INSTALL_DIR="/usr/local/bin"
SERVICE_FILE="/etc/systemd/system/gridflux.service"

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

  echo "Installing gridflux to $INSTALL_DIR..."
  sudo cp gridflux "$INSTALL_DIR/"
  sudo chmod +x "$INSTALL_DIR/gridflux"
}

create_systemd_service() {
  echo "Creating systemd service for gridflux..."

  if [ -f "$SERVICE_FILE" ]; then
    echo "Service already exists. Removing old service..."
    sudo systemctl stop gridflux.service
    sudo systemctl disable gridflux.service
    sudo rm "$SERVICE_FILE"
  fi

  sudo bash -c "cat > $SERVICE_FILE" <<EOL
[Unit]
Description=gridflux window manager
After=graphical.target

[Service]
ExecStart=$INSTALL_DIR/gridflux
User=$USER_NAME
Environment=DISPLAY=:0
Environment=XDG_SESSION_TYPE=$XDG_SESSION_TYPE   # Pass the session type environment variable
Restart=always
RestartSec=3

[Install]
WantedBy=default.target
EOL

  sudo systemctl daemon-reload
  sudo systemctl enable gridflux.service
  sudo systemctl start gridflux.service
}

activate_dynamic_workspaces() {
  if [ "$XDG_CURRENT_DESKTOP" == "GNOME" ]; then
    echo "GNOME desktop detected. Activating dynamic workspaces..."
    gsettings set org.gnome.mutter dynamic-workspaces true
    echo "Dynamic workspaces activated."
  elif [ "$XDG_CURRENT_DESKTOP" == "KDE" ]; then
    echo "KDE desktop detected. Enabling dynamic virtual desktops..."

    # Disable fixed desktop count and allow dynamic desktops
    kwriteconfig5 --file kwinrc NumberOfDesktops 0 # 0 means dynamic desktop count
    kwriteconfig5 --file kwinrc CurrentDesktop 1   # Start with desktop 1

    echo "Dynamic virtual desktops enabled in KDE."
  else
    echo "Dynamic workspaces configuration is not applicable for $XDG_CURRENT_DESKTOP."
  fi
}

echo "Starting installation of gridflux..."

install_dependencies
build_and_install
create_systemd_service
activate_dynamic_workspaces

echo "Installation complete! gridflux is set to run at login."
