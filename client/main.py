import sys
import os

# Ensure the root directory is in the path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from PyQt6.QtWidgets import QApplication
from client.main_window import MainWindow

def main():
    app = QApplication(sys.argv)
    
    # Global application settings could go here
    app.setApplicationName("FuzzerServer Client")
    
    window = MainWindow()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
