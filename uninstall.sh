#!/bin/bash

USER_NAME=$(whoami)
COMPUTER_NAME=$(hostname)

INSTALL_DIR="/usr/local/bin"
SERVICE_FILE="/etc/systemd/system/littlewin.service"

# Function to remove the binary
remove_binary() {
  echo "Removing littlewin binary from $INSTALL_DIR..."
  sudo rm -f "$INSTALL_DIR/littlewin"
}

# Function to remove the systemd service
remove_service() {
  echo "Removing systemd service for littlewin..."

  # Stop and disable the service before removing it
  sudo systemctl stop littlewin.service
  sudo systemctl disable littlewin.service

  # Remove the service file
  sudo rm -f "$SERVICE_FILE"

  # Reload systemd to apply the changes
  sudo systemctl daemon-reload
}

# Main uninstallation process
echo "Starting uninstallation of littlewin..."

remove_binary
remove_service

echo "Uninstallation complete! littlewin has been removed from your system."
