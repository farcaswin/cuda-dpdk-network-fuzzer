from client.config import THEME

def get_stylesheet():
    return f"""
        QMainWindow, QWidget {{
            background-color: {THEME['bg_base']};
            color: {THEME['text']};
            font-family: 'Segoe UI', sans-serif;
        }}

        QLabel {{
            border: none;
        }}

        QLabel#section_header {{
            color: {THEME['accent']};
            font-size: 12pt;
            font-weight: bold;
            margin-bottom: 5px;
        }}

        QPushButton {{
            background-color: {THEME['bg_surface']};
            color: {THEME['text']};
            border: 1px solid {THEME['bg_overlay']};
            padding: 8px 15px;
            border-radius: 4px;
        }}

        QPushButton:hover {{
            background-color: {THEME['bg_overlay']};
        }}

        QPushButton:pressed {{
            background-color: {THEME['accent']};
            color: {THEME['bg_base']};
        }}

        QPushButton:disabled {{
            color: {THEME['text_sub']};
            background-color: {THEME['bg_base']};
        }}

        QPushButton#start_button {{
            background-color: {THEME['green']};
            color: {THEME['bg_base']};
            font-weight: bold;
            font-size: 11pt;
        }}

        QPushButton#stop_button {{
            background-color: {THEME['red']};
            color: {THEME['bg_base']};
            font-weight: bold;
            font-size: 11pt;
        }}

        QPushButton#danger_button {{
            border: 1px solid {THEME['red']};
            color: {THEME['red']};
        }}

        QSplitter::handle {{
            background-color: {THEME['bg_overlay']};
        }}

        QStatusBar {{
            background-color: {THEME['bg_surface']};
            border-top: 1px solid {THEME['bg_overlay']};
            color: {THEME['text_sub']};
        }}

        QGroupBox {{
            border: 1px solid {THEME['bg_overlay']};
            border-radius: 6px;
            margin-top: 1em;
            padding-top: 10px;
            font-weight: bold;
        }}

        QGroupBox::title {{
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 3px 0 3px;
            color: {THEME['accent']};
        }}

        QListWidget {{
            background-color: {THEME['bg_surface']};
            border: 1px solid {THEME['bg_overlay']};
            border-radius: 4px;
        }}

        QListWidget::item {{
            padding: 5px;
            border-bottom: 1px solid {THEME['bg_overlay']};
        }}

        QListWidget::item:selected {{
            background-color: {THEME['bg_overlay']};
            color: {THEME['accent']};
        }}
    """
