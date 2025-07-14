# PrimeCuts - Modular GNOME Shell Search Provider

PrimeCuts is a configurable GNOME Shell search provider that allows you to quickly execute commands, open URLs, and perform various actions directly from the GNOME Shell search interface.

**Note**: This application has been tested specifically on ZorinOS. While it should work on other GNOME-based distributions, compatibility with other desktop environments is not guaranteed.

## Features

- **Modular Configuration**: Organize your actions into logical groups
- **Multiple Action Types**: Support for commands, terminal commands, URLs, and applications
- **Flexible Search**: Search by action name, description, or custom keywords
- **Web Search Integration**: Built-in Google and ChatGPT search options for any search terms
- **Easy Customization**: JSON configuration file for easy editing
- **Icon Support**: Custom icons for each action and group

## Installation

1. Build the project:
```bash
meson setup build
ninja -C build
```

2. Install the binary and configuration files:
```bash
sudo ninja -C build install
```

3. Copy the search provider configuration:
```bash
sudo cp data/de.primeapi.PrimeCuts.search-provider.ini /usr/share/gnome-shell/search-providers/
```

4. Copy the desktop file:
```bash
cp data/primecuts.desktop ~/.local/share/applications/
```

5. Restart GNOME Shell (Alt+F2, type 'r', press Enter)

## Configuration

PrimeCuts uses a JSON configuration file located at `~/.config/primecuts/config.json`. If this file doesn't exist, PrimeCuts will create a default configuration on first run.

### Configuration Structure

The configuration file has the following structure:

```json
{
  "groups": [
    {
      "name": "Group Name",
      "description": "Group description",
      "icon": "group-icon-name",
      "actions": [
        {
          "id": "unique_action_id",
          "name": "Action Name",
          "description": "Action description",
          "icon": "action-icon-name",
          "type": "action_type",
          "command": "command_to_execute",
          "keywords": ["keyword1", "keyword2", "keyword3"]
        }
      ]
    }
  ],
  "global_settings": {
    "terminal_command": "gnome-terminal",
    "browser_command": "xdg-open",
    "enable_notifications": "true"
  }
}
```

### Action Types

PrimeCuts supports four types of actions:

1. **`command`**: Execute a shell command directly
   ```json
   {
     "type": "command",
     "command": "code ."
   }
   ```

2. **`terminal_command`**: Execute a command in a new terminal window
   ```json
   {
     "type": "terminal_command", 
     "command": "ssh user@server.com"
   }
   ```

3. **`url`**: Open a URL in the default browser
   ```json
   {
     "type": "url",
     "command": "https://github.com"
   }
   ```

4. **`application`**: Launch an application
   ```json
   {
     "type": "application",
     "command": "firefox"
   }
   ```

### Example Groups

#### SSH Connections
```json
{
  "name": "SSH Connections",
  "description": "Quick SSH connections to servers",
  "icon": "network-server",
  "actions": [
    {
      "id": "ssh_prod",
      "name": "Production Server",
      "description": "SSH to production server",
      "icon": "network-server",
      "type": "terminal_command",
      "command": "ssh user@prod.example.com",
      "keywords": ["ssh", "prod", "production"]
    }
  ]
}
```

#### Service Management
```json
{
  "name": "Services",
  "description": "System service management",
  "icon": "applications-system",
  "actions": [
    {
      "id": "restart_apache",
      "name": "Restart Apache",
      "description": "Restart Apache web server",
      "icon": "applications-internet",
      "type": "terminal_command",
      "command": "sudo systemctl restart apache2",
      "keywords": ["apache", "restart", "web"]
    }
  ]
}
```

#### Development Tools
```json
{
  "name": "Development",
  "description": "Development tools and shortcuts",
  "icon": "applications-development",
  "actions": [
    {
      "id": "git_status",
      "name": "Git Status", 
      "description": "Show git repository status",
      "icon": "git",
      "type": "terminal_command",
      "command": "git status",
      "keywords": ["git", "status", "repo"]
    }
  ]
}
```

#### Quick Links
```json
{
  "name": "Websites",
  "description": "Quick access to websites",
  "icon": "applications-internet",
  "actions": [
    {
      "id": "github",
      "name": "GitHub",
      "description": "Open GitHub in browser",
      "icon": "github",
      "type": "url",
      "command": "https://github.com",
      "keywords": ["github", "git", "repo"]
    }
  ]
}
```

## Usage

1. Press the Super key or click the Activities button to open GNOME Shell search
2. Type keywords related to your configured actions
3. Select the desired action from the search results
4. Press Enter to execute the action

### Web Search Integration

PrimeCuts automatically adds web search options for any search query:

- **Google Search**: When you type any search terms, "Search on Google: [your terms]" will appear as an option
- **ChatGPT Search**: Similarly, "Search on ChatGPT: [your terms]" will be available

These options appear at the end of your search results, allowing you to quickly search the web for anything that doesn't match your configured actions.

## Global Settings

The `global_settings` section allows you to customize PrimeCuts behavior:

- **`terminal_command`**: The terminal emulator to use for terminal commands (default: "gnome-terminal")
- **`browser_command`**: The command to open URLs (default: "xdg-open")
- **`enable_notifications`**: Whether to show notifications (default: "true")

## Icon Names

You can use any valid icon name from your system's icon theme. Common icon names include:

### System Icons
- `utilities-terminal`
- `applications-internet`
- `network-server`
- `applications-development`
- `applications-system`
- `applications-databases`
- `folder`
- `document-open`
- `system-run`
- `web-browser`

### Finding Available Icons

To find available icons on your system, you can:

1. **Browse system icon directories**:
   ```bash
   ls /usr/share/icons/*/16x16/apps/
   ls /usr/share/icons/*/22x22/apps/
   ```

2. **Use the Freedesktop Icon Naming Specification**: Visit [specifications.freedesktop.org](https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html) for standard icon names

3. **Check your desktop environment's icon theme**: Most icons follow standard naming conventions like:
   - `applications-*` (e.g., `applications-internet`, `applications-games`)
   - `network-*` (e.g., `network-server`, `network-wireless`)
   - `utilities-*` (e.g., `utilities-terminal`, `utilities-system-monitor`)
   - `folder-*` (e.g., `folder-documents`, `folder-downloads`)

## Debugging

Run PrimeCuts with debug output:

```bash
./build/primecuts --debug
```

This will show detailed information about configuration loading, search requests, and action execution.

## Tips

1. **Organize by workflow**: Group related actions together (e.g., all SSH connections, all service restarts)
2. **Use descriptive keywords**: Include multiple ways someone might search for an action
3. **Test your commands**: Make sure commands work in a terminal before adding them to the configuration
4. **Use meaningful icons**: Icons help quickly identify actions in search results
5. **Keep descriptions concise**: They appear in search results, so make them informative but brief
6. **Leverage web search**: Don't worry about covering every possible search - the built-in Google and ChatGPT search will handle anything else

## Troubleshooting

- **Actions don't appear in search**: Check that the search provider is properly installed and GNOME Shell has been restarted
- **Actions don't execute**: Run with `--debug` to see error messages
- **Configuration not loading**: Check JSON syntax and file permissions
- **Terminal commands don't work**: Verify your terminal_command setting in global_settings
