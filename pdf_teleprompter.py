
import tkinter as tk
from tkinter import filedialog, messagebox

import pymupdf
from PIL import Image, ImageTk


class PDFTeleprompter:
    def __init__(self, root):
        self.root = root

        # --------------------------------------------------
        # Select PDF file
        # --------------------------------------------------
        pdf_file = filedialog.askopenfilename(
            title="Select Teleprompter PDF",
            filetypes=[("PDF Files", "*.pdf")]
        )

        if not pdf_file:
            root.destroy()
            return

        # --------------------------------------------------
        # Open PDF
        # --------------------------------------------------
        try:
            self.doc = pymupdf.open(pdf_file)
        except Exception as e:
            messagebox.showerror(
                "PDF Error",
                f"Could not open PDF.\n\n{e}"
            )
            root.destroy()
            return

        self.page_number = 0
        self.total_pages = len(self.doc)

        # --------------------------------------------------
        # Full screen
        # --------------------------------------------------
        self.fullscreen = True

        self.root.title("PDF Teleprompter")
        self.root.configure(bg="black")
        self.root.attributes("-fullscreen", True)

        # --------------------------------------------------
        # Canvas
        # --------------------------------------------------
        self.canvas = tk.Canvas(
            root,
            bg="black",
            highlightthickness=0
        )

        self.canvas.pack(
            fill=tk.BOTH,
            expand=True
        )

        # Keep PhotoImage reference
        self.photo = None

        # --------------------------------------------------
        # Keyboard controls
        # --------------------------------------------------
        self.root.bind("<Right>", self.next_page)
        self.root.bind("<Left>", self.previous_page)

        self.root.bind("<Next>", self.next_page)       # Page Down
        self.root.bind("<Prior>", self.previous_page)  # Page Up

        self.root.bind("<space>", self.next_page)

        self.root.bind("<Escape>", self.exit_program)
        self.root.bind("<f>", self.toggle_fullscreen)
        self.root.bind("<F>", self.toggle_fullscreen)

        # Redraw if window size changes
        self.root.bind("<Configure>", self.window_changed)

        # --------------------------------------------------
        # Display first page
        # --------------------------------------------------
        self.root.after(200, self.show_page)

    # ======================================================
    # Display PDF page
    # ======================================================
    def show_page(self, event=None):

        if self.total_pages == 0:
            return

        page = self.doc.load_page(self.page_number)

        # Get current screen/window size
        screen_width = self.root.winfo_width()
        screen_height = self.root.winfo_height()

        if screen_width < 100 or screen_height < 100:
            screen_width = self.root.winfo_screenwidth()
            screen_height = self.root.winfo_screenheight()

        page_rect = page.rect

        # Calculate zoom so entire PDF page fits on screen
        zoom_x = screen_width / page_rect.width
        zoom_y = screen_height / page_rect.height

        zoom = min(zoom_x, zoom_y)

        matrix = pymupdf.Matrix(zoom, zoom)

        # Render PDF page
        pix = page.get_pixmap(
            matrix=matrix,
            colorspace=pymupdf.csRGB,
            alpha=False
        )

        # Convert PyMuPDF pixmap to PIL image
        image = Image.frombytes(
            "RGB",
            (pix.width, pix.height),
            pix.samples
        )

        # Convert PIL image for Tkinter
        self.photo = ImageTk.PhotoImage(image)

        # Clear old page
        self.canvas.delete("all")

        # Center page on screen
        x = screen_width // 2
        y = screen_height // 2

        self.canvas.create_image(
            x,
            y,
            image=self.photo,
            anchor=tk.CENTER
        )

        # Small page indicator
        self.canvas.create_text(
            20,
            20,
            anchor="nw",
            text=f"{self.page_number + 1} / {self.total_pages}",
            fill="red",
            font=("Arial", 18, "bold")
        )

    # ======================================================
    # NEXT
    # ======================================================
    def next_page(self, event=None):

        if self.page_number < self.total_pages - 1:
            self.page_number += 1
            self.show_page()

    # ======================================================
    # PREVIOUS
    # ======================================================
    def previous_page(self, event=None):

        if self.page_number > 0:
            self.page_number -= 1
            self.show_page()

    # ======================================================
    # ESC = Exit
    # ======================================================
    def exit_program(self, event=None):

        try:
            self.doc.close()
        except:
            pass

        self.root.destroy()

    # ======================================================
    # F = Fullscreen ON / OFF
    # ======================================================
    def toggle_fullscreen(self, event=None):

        self.fullscreen = not self.fullscreen

        self.root.attributes(
            "-fullscreen",
            self.fullscreen
        )

    # ======================================================
    # Window resized
    # ======================================================
    def window_changed(self, event=None):

        if not self.fullscreen:
            self.root.after(100, self.show_page)


# ==========================================================
# MAIN
# ==========================================================

root = tk.Tk()

app = PDFTeleprompter(root)

root.mainloop()