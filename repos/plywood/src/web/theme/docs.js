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

function navigateTo(path, forward) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("article").innerHTML = this.responseText;
            window.scrollTo(0, 0);

            // Select appropriate TOC entry
            var selected = document.querySelector(".sidebar").getElementsByTagName("li");
            for (var j = 0; j < selected.length; j++) {
                var li = selected[j];
                var mustSelect = (li.parentElement.getAttribute("href") == path);
                li.classList.remove(mustSelect ? "unselected" : "selected");
                li.classList.add(mustSelect ? "selected" : "unselected");
            }

            // Update history
            if (forward) {
                history.pushState(null, null, path);
            }
        }
    };
    xhttp.open("GET", "/content?path=" + path, true);
    xhttp.send();            
}

window.onload = function() { 
    highlight(location.hash.substr(1));

    var list = document.getElementsByClassName("caret");
    for (var i = 0; i < list.length; i++) {
        list[i].addEventListener("click", function() {
            this.classList.toggle("caret-down");
            var childList = this.nextElementSibling || this.parentElement.nextElementSibling;
            childList.classList.toggle("active");
        });
    }

    var list = document.querySelector(".sidebar").getElementsByTagName("a");
    for (var i = 0; i < list.length; i++) {
        var a = list[i];
        var path = a.getAttribute("href");
        if (path.substring(0, 6) == "/docs/") {
            a.onclick = function() {
                navigateTo(this.getAttribute("href"), true);
                return false;
            }
        }    
    }
}

window.onhashchange = function() { 
    highlight(location.hash.substr(1));
}

window.addEventListener("popstate", function(e) {
    navigateTo(location.pathname, false);
});
