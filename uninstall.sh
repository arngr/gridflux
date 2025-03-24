#!/bin/bash

USER_NAME=$(whoami)
COMPUTER_NAME=$(hostname)

INSTALL_DIR="/usr/local/bin"
SERVICE_FILE="/etc/systemd/system/gridflux.service"

remove_binary() {
  echo "Removing gridflux binary from $INSTALL_DIR..."
  if [ -f "$INSTALL_DIR/gridflux" ]; then
    sudo rm -f "$INSTALL_DIR/gridflux"
    echo "Binary removed successfully."
  else
    echo "Binary not found at $INSTALL_DIR. Skipping."
  fi
}

remove_service() {
  echo "Removing systemd service for gridflux..."

  if [ -f "$SERVICE_FILE" ]; then
    sudo systemctl stop gridflux.service
    sudo systemctl disable gridflux.service
    sudo rm -f "$SERVICE_FILE"

    # Reload systemd to apply the changes
    sudo systemctl daemon-reload
    echo "Systemd service removed successfully."
  else
    echo "Systemd service file not found. Skipping."
  fi
}

echo "Starting uninstallation of gridflux..."

remove_binary
remove_service

echo "Uninstallation complete! gridflux has been removed from your system."
