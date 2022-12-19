# SpaceShooter

The project was meant to try out and prototype several ECS-related features, such as iteration through entities, components, signature comparison, etc.
The components are sequential in the data segment of the application; therefore, the only dynamic allocations are resources. In this case textures.

The gamae itself is rather flat - The player can fly, shoot, and the enemies just wiggle back and forth.
The idea behind this application was working as much as possible with the data-segment and the features, while my implementation allowed data-driven operations on the data. Creation of data entry, update, querying and filtering.

Everything got implemented as much as possible through template meta programming what allowed the signature creation and comparisons to happen during compile-time.
Certain For-Loops are redundant and should use Where and For with container argument instead. But so far the framework allows refactoring like that. 
