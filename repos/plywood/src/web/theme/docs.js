var highlighted = null;

function highlight(elementID) {
    if (highlighted) {
        highlighted.style.background = "";
    }
    highlighted = document.getElementById(elementID);
    if (highlighted) {
        highlighted.style.background = "#ffffa0";
    }
}

window.onload = function() { 
    highlight(location.hash.substr(1));
    var defTitles = document.getElementsByClassName("defTitle");
    for (var i = 0; i < defTitles.length; i++) {
        defTitles[i].onmouseenter = function(e) {
            var linkElems = e.target.getElementsByClassName("headerlink");
            for (var j = 0; j < linkElems.length; j++) {
                linkElems[j].style.visibility = "visible";
            }
        }
        defTitles[i].onmouseleave = function(e) {
            var linkElems = e.target.getElementsByClassName("headerlink");
            for (var j = 0; j < linkElems.length; j++) {
                linkElems[j].style.visibility = "hidden";
            }
        }
    }

    var toggler = document.getElementsByClassName("caret");
    for (var i = 0; i < toggler.length; i++) {
      toggler[i].addEventListener("click", function() {
        this.classList.toggle("caret-down");
        this.nextElementSibling.classList.toggle("active");
      });
    }
}

window.onhashchange = function() { 
    highlight(location.hash.substr(1));
}
