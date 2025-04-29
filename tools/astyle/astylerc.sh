# Use tabs
-t 
# Use break braces style
--style=break 
# Break the line at 120 characters
--max-code-length=120 


# add a line after brackets
-F
# Remove braces from conditional statements
-xj 
# Remove redundant whitespace
--squeeze-ws 
# Don't allow more than 2 empty lines
--squeeze-lines=2 
# Insert space padding around operators.
-p
# Insert space padding after commas. 
-xg 
# Remove padding around square brackets on both the outside and the inside.
--unpad-brackets 
# Break one line headers (e.g. 'if', 'while', 'else', ...) from a statement residing on the same line. 
-xb 
# Indent C++ comments beginning in column one.
-Y 
# Fill empty lines with whitespace
-E