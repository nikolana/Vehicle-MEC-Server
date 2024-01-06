from fpdf import FPDF

# Creating a class for the CV
class PDF(FPDF):
    def header(self):
        self.set_font('Arial', 'B', 12)
        self.cell(0, 10, 'Curriculum Vitae', 0, 1, 'C')

    def chapter_title(self, title):
        self.set_font('Arial', 'B', 12)
        self.cell(0, 10, title, 0, 1, 'L')
        self.ln(2)

    def chapter_body(self, body):
        self.set_font('Arial', '', 12)
        self.multi_cell(0, 10, body)
        self.ln()

# Instantiating the PDF class
pdf = PDF()

# Adding a page
pdf.add_page()

# Adding content
pdf.chapter_title('Personal Details')
pdf.chapter_body('Name: Anastas Nikolov\n'
                 'Address: Litvínov, Czech Republic\n'
                 'Phone: +420776191090\n'
                 'Email: nikolana@fel.cvut.cz\n'
                 'LinkedIn: https://www.linkedin.com/in/anastas-nikolov-14482323a/')

pdf.chapter_title('Professional Summary')
pdf.chapter_body('A motivated and skilled software engineer with experience in Python, Matlab, C/C++, and Bash. '
                 'Proven track record in deep neural network research and test automation in a high-tech automotive setting.')

pdf.chapter_title('Skills')
pdf.chapter_body('Python (Advanced)\n'
                 'Matlab (Advanced)\n'
                 'C (Intermediate)\n'
                 'C++ (Intermediate)\n'
                 'Bash (Intermediate)\n'
                 'Git (Advanced)\n'
                 'ASPICE (Intermediate)')

pdf.chapter_title('Professional Experience')
pdf.chapter_body('Research Assistant\n'
                 'Faculty of Electrical Engineering, Czech Technical University in Prague\n'
                 'March 2021 - December 2023\n'
                 '- Channel quality prediction using deep neural network (Python)\n\n'
                 'Software Testing Team Intern\n'
                 'Porsche Engineering Services\n'
                 'July 2023 - Present\n'
                 '- Focused on HiL test automation (Python + Bash), web scraping (Python), and Test Environment Installation scripts (Bash)')

pdf.chapter_title('Certifications')
pdf.chapter_body('FCE Cambridge Certificate')

pdf.chapter_title('Languages')
pdf.chapter_body('English (Professional Level)\n'
                 'Czech (Native Speaker)\n'
                 'German (Passive)')

pdf.chapter_title('Hobbies')
pdf.chapter_body('Football, Climbing, Skiing, Reading')

# Output the file
cv_path = "Anastas_Nikolov_CV.pdf"
pdf.output(cv_path)

cv_path

