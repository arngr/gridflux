#!/bin/bash

USER_NAME=$(whoami)
COMPUTER_NAME=$(hostname)

INSTALL_DIR="/usr/local/bin"
SERVICE_FILE="/etc/systemd/system/littlewin.service"

remove_binary() {
  echo "Removing littlewin binary from $INSTALL_DIR..."
  if [ -f "$INSTALL_DIR/littlewin" ]; then
    sudo rm -f "$INSTALL_DIR/littlewin"
    echo "Binary removed successfully."
  else
    echo "Binary not found at $INSTALL_DIR. Skipping."
  fi
}

remove_service() {
  echo "Removing systemd service for littlewin..."

  if [ -f "$SERVICE_FILE" ]; then
    sudo systemctl stop littlewin.service
    sudo systemctl disable littlewin.service
    sudo rm -f "$SERVICE_FILE"

    # Reload systemd to apply the changes
    sudo systemctl daemon-reload
    echo "Systemd service removed successfully."
  else
    echo "Systemd service file not found. Skipping."
  fi
}

echo "Starting uninstallation of littlewin..."

remove_binary
remove_service

echo "Uninstallation complete! littlewin has been removed from your system."
