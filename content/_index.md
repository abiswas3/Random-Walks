---
title: "Ari Biswas"
date: 2025-05-15
showtitle: false
---

<div class="home-container">
  <div class="photo-container">
    <img id="random-photo" alt="Random Photo" />
  </div>
  <div class="text-container">

I received my undergraduate degree in Computer Engineering and Applied Mathematics and Graduate Degree in Computer Science from the 
[University of Wisconsin-Madison](https://www.wisc.edu). From 2017-2023, I was employed by Amazon under the supervision of 
[Ben Snyder](https://www.linkedin.com/in/benjamin-snyder-3b777b10) and 
[Daniel Marcu](https://www.linkedin.com/in/daniel-marcu-9505864/), 
as a scientist working on Amazon and Alexa search. In July 2022, I decided to switch focus from machine learning and pursue a doctoral degree in theoretical computer science under the supervision of 
[Graham Cormode](https://warwick.ac.uk/fac/sci/dcs/people/graham_cormode/) and 
[Rajko Nenadov](https://sites.google.com/view/rajkon) at the University of Warwick. 
I was awarded the 
[Chancellors International Scholarship](https://warwick.ac.uk/services/dc/schols_fund/scholarships_and_funding/chancellors_int/#:~:text=Eligibility,to%20begin%20in%20October%202022%3B&amp;text=Applicants%20may%20be%20from%20any,to%20the%20next%20academic%20year.), which supports my doctoral studies.


  <div class="hero-links" style="margin-top: 1em;">
    <a href="https://github.com/abiswas3" target="_blank" rel="noopener" class="hero-link">GitHub</a> &nbsp;|&nbsp;
    <a href="https://scholar.google.com/citations?user=yGuM3MgAAAAJ&hl=en" target="_blank" rel="noopener" class="hero-link">Google Scholar</a>
  </div>
  
  </div>
</div>

<script>
  const images = [
    '/images/profile_pics/me.jpg',
    '/images/profile_pics/me_running.jpeg',
    '/images/profile_pics/me2.jpg',
    '/images/profile_pics/me5.jpg',
    '/images/profile_pics/me4_oxford.JPG',
    '/images/profile_pics/me5.jpg',
    '/images/profile_pics/me_boston.jpeg',
    '/images/profile_pics/me7.JPG',
  ];

  document.addEventListener('DOMContentLoaded', () => {
    const img = document.getElementById('random-photo');
    const randomIndex = Math.floor(Math.random() * images.length);
    img.src = images[randomIndex];
  });
</script>
