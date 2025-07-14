#!/bin/bash

# Install script for PrimeCuts search provider

echo "Installing PrimeCuts search provider..."

# Create directories if they don't exist
mkdir -p ~/.local/share/applications
mkdir -p ~/.local/share/gnome-shell/search-providers

# Copy the desktop file
echo "Installing desktop file..."
cp data/primecuts.desktop ~/.local/share/applications/

# Copy the search provider configuration
echo "Installing search provider configuration..."
cp data/de.primeapi.PrimeCuts.search-provider.ini /usr/share/gnome-shell/search-providers/

# Copy the search provider configuration
#echo "Installing dbus service file..."
#cp data/de.primeapi.PrimeCuts.service ~/.local/share/gnome-shell/search-providers/

# Update desktop database
echo "Updating desktop database..."
update-desktop-database ~/.local/share/applications

echo "Installation complete!"
echo ""
echo "To register with GNOME Shell, you may need to:"
echo "1. Restart GNOME Shell (Alt+F2, type 'r', press Enter)"
echo "2. Or log out and log back in"
echo ""
echo "Then run your service with: ./build/primecuts --debug"
